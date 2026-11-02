#include "core/Config.hpp"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "utils/Logger.hpp"

namespace fs = std::filesystem;

Config::Config() {
    fs::path config_dir = fs::path(CONFIG_PATH).parent_path();
    if (!fs::exists(config_dir)) {
        fs::create_directories(config_dir);
    }
}

bool Config::load() {
    try {
        std::ifstream file(CONFIG_PATH);
        if (!file.is_open()) {
            Logger::info("Config file not found, using defaults");
            return true;
        }

        file >> config_;
        load_from_json();
        Logger::info("Config loaded from {}", CONFIG_PATH);
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to load config: {}", e.what());
        return false;
    }
}

bool Config::save() {
    try {
        write_to_json();
        std::ofstream file(CONFIG_PATH);
        if (!file.is_open()) {
            Logger::error("Failed to open config file for writing");
            return false;
        }

        file << config_.dump(4);
        Logger::info("Config saved to {}", CONFIG_PATH);
        return true;
    } catch (const std::exception& e) {
        Logger::error("Failed to save config: {}", e.what());
        return false;
    }
}

bool Config::has_valid_cookies() const {
    return !sessdata_.empty();
}

void Config::set_cookies(const std::string& sessdata, const std::string& bili_jct, const std::string& buvid3) {
    sessdata_ = sessdata;
    bili_jct_ = bili_jct;
    buvid3_ = buvid3;
    config_["cookies"]["SESSDATA"] = sessdata;
    config_["cookies"]["bili_jct"] = bili_jct;
    config_["cookies"]["buvid3"] = buvid3;
}

void Config::load_from_json() {
    if (config_.contains("cookies")) {
        sessdata_ = config_["cookies"].value("SESSDATA", "");
        bili_jct_ = config_["cookies"].value("bili_jct", "");
        buvid3_ = config_["cookies"].value("buvid3", "");
    }

    playback_speed_ = config_.value("playback_speed", 1.0f);
    sleep_timer_minutes_ = config_.value("sleep_timer_minutes", 0);
    auto_play_next_ = config_.value("auto_play_next", true);
    background_playback_ = config_.value("background_playback", true);
    volume_ = config_.value("volume", 0.8f);

    theme_ = config_.value("theme", "dark");
    font_size_ = config_.value("font_size", "medium");

    download_path_ = config_.value("download_path", "/switch/switch-audiobook/downloads/");
    max_concurrent_downloads_ = config_.value("max_concurrent_downloads", 3);
    wifi_only_download_ = config_.value("wifi_only_download", true);

    http_timeout_ = config_.value("http_timeout", 15000);
    verify_ssl_ = config_.value("verify_ssl", true);

    allow_analytics_ = config_.value("allow_analytics", false);
}

void Config::write_to_json() {
    config_["cookies"] = {
        {"SESSDATA", sessdata_},
        {"bili_jct", bili_jct_},
        {"buvid3", buvid3_}
    };

    config_["playback_speed"] = playback_speed_;
    config_["sleep_timer_minutes"] = sleep_timer_minutes_;
    config_["auto_play_next"] = auto_play_next_;
    config_["background_playback"] = background_playback_;
    config_["volume"] = volume_;

    config_["theme"] = theme_;
    config_["font_size"] = font_size_;

    config_["download_path"] = download_path_;
    config_["max_concurrent_downloads"] = max_concurrent_downloads_;
    config_["wifi_only_download"] = wifi_only_download_;

    config_["http_timeout"] = http_timeout_;
    config_["verify_ssl"] = verify_ssl_;

    config_["allow_analytics"] = allow_analytics_;
}

void Config::clear_all_data() {
    config_.clear();
    sessdata_.clear();
    bili_jct_.clear();
    buvid3_.clear();
    *this = Config();
}