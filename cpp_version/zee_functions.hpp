#ifndef ZEE_FUNCTIONS_HPP
#define ZEE_FUNCTIONS_HPP

#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <regex>
#include "base64.h"

// Custom Base64Encode function
std::string Base64Encode(const std::string& input) {
    return base64_encode(reinterpret_cast<const unsigned char*>(input.c_str()), input.length());
}

// Function to generate the DD token
std::string generateDDToken() {
    nlohmann::json dd_token_data = {
        {"schema_version", "1"},
        {"os_name", "N/A"},
        {"os_version", "N/A"},
        {"platform_name", "Chrome"},
        {"platform_version", "104"},
        {"device_name", ""},
        {"app_name", "Web"},
        {"app_version", "2.52.31"},
        {"player_capabilities", {
            {"audio_channel", {"STEREO"}},
            {"video_codec", {"H264"}},
            {"container", {"MP4", "TS"}},
            {"package", {"DASH", "HLS"}},
            {"resolution", {"240p", "SD", "HD", "FHD"}},
            {"dynamic_range", {"SDR"}}
        }},
        {"security_capabilities", {
            {"encryption", {"WIDEVINE_AES_CTR"}},
            {"widevine_security_level", {"L3"}},
            {"hdcp_version", {"HDCP_V1", "HDCP_V2", "HDCP_V2_1", "HDCP_V2_2"}}
        }}
    };
    return Base64Encode(dd_token_data.dump());
}

#include <stdexcept>

class ZeeException : public std::runtime_error {
public:
    ZeeException(const std::string& message) : std::runtime_error(message) {}
};

#include <random>
#include <sstream>

// Function to generate a guest token
std::string generateGuestToken() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4"; // UUID version 4
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << (dis(gen) & 0x3 | 0x8); // Variant
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

// Function to fetch the platform token
std::string fetchPlatformToken() {
    cpr::Response r = cpr::Get(cpr::Url{"https://www.zee5.com/live-tv/aaj-tak/0-9-aajtak"},
                               cpr::Header{{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36"}});
    if (r.status_code != 200) {
        throw ZeeException("Failed to fetch platform token: " + r.text);
    }

    std::regex re("window\\.appConfig=({.*?});");
    std::smatch match;
    if (std::regex_search(r.text, match, re) && match.size() > 1) {
        try {
            auto json_response = nlohmann::json::parse(match[1].str());
            if (json_response.contains("gwapiPlatformToken")) {
                return json_response["gwapiPlatformToken"];
            }
        } catch (const nlohmann::json::parse_error& e) {
            throw ZeeException("Failed to parse JSON for platform token: " + std::string(e.what()));
        }
    }

    throw ZeeException("Platform token not found in response.");
}

// Function to fetch the M3U8 URL
std::string fetchM3U8url(const std::string& channelId) {
    std::string guestToken = generateGuestToken();
    std::string platformToken = fetchPlatformToken();

    nlohmann::json payload = {
        {"x-access-token", platformToken},
        {"X-Z5-Guest-Token", guestToken},
        {"x-dd-token", generateDDToken()}
    };

    cpr::Response r = cpr::Post(cpr::Url{"https://spapi.zee5.com/singlePlayback/getDetails/secure?channel_id=" + channelId + "&device_id=" + guestToken + "&platform_name=desktop_web&translation=en&user_language=en,hi,te&country=IN&state=&app_version=4.24.0&user_type=guest&check_parental_control=false"},
                                cpr::Header{{"Content-Type", "application/json"}},
                                cpr::Body{payload.dump()});

    if (r.status_code != 200) {
        throw ZeeException("Failed to fetch M3U8 URL: " + r.text);
    }

    try {
        auto json_response = nlohmann::json::parse(r.text);
        if (json_response.contains("keyOsDetails") && json_response["keyOsDetails"].contains("video_token")) {
            return json_response["keyOsDetails"]["video_token"];
        }
    } catch (const nlohmann::json::parse_error& e) {
        throw ZeeException("Failed to parse JSON for M3U8 URL: " + std::string(e.what()));
    }

    throw ZeeException("video_token not found in response for M3U8 URL.");
}

// Function to generate the Zee5 cookie
std::string generateCookieZee5(const std::string& userAgent, const std::string& channelId) {
    std::string m3u8Url = fetchM3U8url(channelId);

    if (m3u8Url.find("https://") != 0) {
        throw ZeeException("Invalid M3U8 URL received: " + m3u8Url);
    }

    cpr::Response r = cpr::Get(cpr::Url{m3u8Url},
                               cpr::Header{{"User-Agent", userAgent}});

    if (r.status_code != 200) {
        throw ZeeException("Failed to fetch cookie: " + r.text);
    }

    std::regex re("hdntl=([^\\s\"]+)");
    std::smatch match;
    if (std::regex_search(r.text, match, re) && match.size() > 1) {
        return match[0].str();
    }

    throw ZeeException("Cookie not found in response.");
}

#endif // ZEE_FUNCTIONS_HPP
