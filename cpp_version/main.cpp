#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "zee_functions.hpp"

using json = nlohmann::json;

// Function to get the Zee5 cookie, with caching
std::string getCookieZee5(const std::string& userAgent, const std::string& channelId) {
    // Basic caching mechanism
    std::string cacheFile = "cookie_cache_" + channelId + ".txt";
    std::ifstream inFile(cacheFile);
    if (inFile.is_open()) {
        std::string cookie;
        std::getline(inFile, cookie);
        inFile.close();
        // Here you might want to add expiry logic
        return cookie;
    }

    std::string cookie = generateCookieZee5(userAgent, channelId);
    std::ofstream outFile(cacheFile);
    if (outFile.is_open()) {
        outFile << cookie;
        outFile.close();
    }
    return cookie;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> countries;
    std::vector<std::string> languages;

    // Simple argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--country=", 0) == 0) {
            countries.push_back(arg.substr(10));
        } else if (arg.rfind("--language=", 0) == 0) {
            languages.push_back(arg.substr(11));
        }
    }

    std::ifstream f("../../data.json");
    if (!f.is_open()) {
        std::cerr << "Could not open data.json" << std::endl;
        return 1;
    }
    json data = json::parse(f);

    std::ofstream playlistFile("playlist.m3u");
    if (!playlistFile.is_open()) {
        std::cerr << "Could not create playlist.m3u" << std::endl;
        return 1;
    }

    playlistFile << "#EXTM3U" << std::endl;
    playlistFile << "#https://github.com/yuvraj824/zee5" << std::endl << std::endl;

    std::string userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36";

    for (const auto& channel : data["data"]) {
        try {
            std::string channelId = channel.value("id", "");
            if (channelId.empty()) {
                continue;
            }
            std::string cookie = getCookieZee5(userAgent, channelId);

            bool countryMatch = countries.empty();
        for (const auto& c : countries) {
            if (channel["country"] == c) {
                countryMatch = true;
                break;
            }
        }

        bool languageMatch = languages.empty();
        for (const auto& l : languages) {
            if (channel["language"] == l) {
                languageMatch = true;
                break;
            }
        }

        if (countryMatch && languageMatch) {
            playlistFile << "#EXTINF:-1 tvg-id=\"" << channel.value("id", "") << "\" tvg-country=\"" << channel.value("country", "")
                         << "\" tvg-language=\"" << channel.value("language", "") << "\" tvg-name=\"" << channel.value("name", "")
                         << "\" tvg-logo=\"" << channel.value("logo", "") << "\" group-title=\"" << channel.value("genre", "")
                         << "\", " << channel.value("name", "") << std::endl;
            playlistFile << channel.value("url", "") << "?" << cookie << std::endl;
        }
        } catch (const ZeeException& e) {
            std::cerr << "Error processing channel " << channel.value("id", "") << ": " << e.what() << std::endl;
        }
    }

    std::cout << "Playlist generated successfully." << std::endl;

    return 0;
}
