#include <httplib.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "zee_functions.hpp"

using json = nlohmann::json;

int main() {
    httplib::Server svr;

    svr.Get("/playlist", [](const httplib::Request& req, httplib::Response& res) {
        std::vector<std::string> countries;
        if (req.has_param("country")) {
            std::string country_param = req.get_param_value("country");
            // Split by comma
            std::stringstream ss(country_param);
            std::string item;
            while (std::getline(ss, item, ',')) {
                countries.push_back(item);
            }
        }

        std::vector<std::string> languages;
        if (req.has_param("language")) {
            std::string language_param = req.get_param_value("language");
            // Split by comma
            std::stringstream ss(language_param);
            std::string item;
            while (std::getline(ss, item, ',')) {
                languages.push_back(item);
            }
        }

        std::ifstream f("../../data.json");
        if (!f.is_open()) {
            res.status = 500;
            res.set_content("Could not open data.json", "text/plain");
            return;
        }
        json data = json::parse(f);

        std::stringstream playlist;
        playlist << "#EXTM3U" << std::endl;
        playlist << "#https://github.com/yuvraj824/zee5" << std::endl << std::endl;

        for (const auto& channel : data["data"]) {
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
                playlist << "#EXTINF:-1 tvg-id=\"" << channel.value("id", "") << "\" tvg-country=\"" << channel.value("country", "")
                         << "\" tvg-language=\"" << channel.value("language", "") << "\" tvg-name=\"" << channel.value("name", "")
                         << "\" tvg-logo=\"" << channel.value("logo", "") << "\" group-title=\"" << channel.value("genre", "")
                         << "\", " << channel.value("name", "") << std::endl;
                playlist << "http://" << req.local_addr << ":" << req.local_port << "/stream?id=" << channel.value("id", "") << std::endl;
            }
        }

        res.set_content(playlist.str(), "audio/x-mpegurl");
    });

    svr.Get("/stream", [](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Channel ID not found in query parameter.", "text/plain");
            return;
        }
        std::string channelId = req.get_param_value("id");
        std::string userAgent = req.get_header_value("User-Agent");
        if (userAgent.empty()) {
            userAgent = "Mozilla/5.0";
        }

        try {
            std::string cookie = generateCookieZee5(userAgent, channelId);

            std::ifstream f("../../data.json");
            if (!f.is_open()) {
                res.status = 500;
                res.set_content("Could not open data.json", "text/plain");
                return;
            }
            json data = json::parse(f);

            for (const auto& channel : data["data"]) {
                if (channel["id"] == channelId) {
                    std::string streamUrl = channel.value("url", "");
                    res.set_redirect((streamUrl + "?" + cookie).c_str());
                    return;
                }
            }

            res.status = 404;
            res.set_content("Channel not found.", "text/plain");
        } catch (const ZeeException& e) {
            res.status = 500;
            res.set_content(e.what(), "text/plain");
        }
    });

    std::cout << "Starting server on port 8080..." << std::endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}
