#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <borealis.hpp>

class Config {
public:
    Config();
    ~Config() = default;

    static constexpr const char* CONFIG_PATH = "/switch/switch-audiobook/config.json";

    bool load();
    bool save();

    bool has_valid_cookies() const;
    std::string get_sessdata() const { return sessdata_; }
    std::string get_bili_jct() const { return bili_jct_; }
    std::string get_buvid3() const { return buvid3_; }
    void set_cookies(const std::string& sessdata, const std::string& bili_jct, const std::string& buvid3);

    float get_playback_speed() const { return playback_speed_; }
    void set_playback_speed(float speed) { playback_speed_ = std::clamp(speed, 0.5f, 3.0f); }

    int get_sleep_timer_minutes() const { return sleep_timer_minutes_; }
    void set_sleep_timer_minutes(int minutes) { sleep_timer_minutes_ = minutes; }

    bool get_auto_play_next() const { return auto_play_next_; }
    void set_auto_play_next(bool enabled) { auto_play_next_ = enabled; }

    bool get_background_playback() const { return background_playback_; }
    void set_background_playback(bool enabled) { background_playback_ = enabled; }

    float get_volume() const { return volume_; }
    void set_volume(float vol) { volume_ = std::clamp(vol, 0.0f, 1.0f); }

    std::string get_theme() const { return theme_; }
    void set_theme(const std::string& theme) { theme_ = theme; }

    std::string get_font_size() const { return font_size_; }
    void set_font_size(const std::string& size) { font_size_ = size; }

    std::string get_download_path() const { return download_path_; }
    void set_download_path(const std::string& path) { download_path_ = path; }

    int get_max_concurrent_downloads() const { return max_concurrent_downloads_; }
    void set_max_concurrent_downloads(int n) { max_concurrent_downloads_ = std::clamp(n, 1, 5); }

    bool get_wifi_only_download() const { return wifi_only_download_; }
    void set_wifi_only_download(bool enabled) { wifi_only_download_ = enabled; }

    int get_http_timeout() const { return http_timeout_; }
    void set_http_timeout(int ms) { http_timeout_ = std::clamp(ms, 5000, 60000); }

    bool get_verify_ssl() const { return verify_ssl_; }
    void set_verify_ssl(bool verify) { verify_ssl_ = verify; }

    bool get_allow_analytics() const { return allow_analytics_; }
    void set_allow_analytics(bool allowed) { allow_analytics_ = allowed; }

    void clear_all_data();

private:
    nlohmann::json config_;
    std::string sessdata_;
    std::string bili_jct_;
    std::string buvid3_;

    float playback_speed_ = 1.0f;
    int sleep_timer_minutes_ = 0;
    bool auto_play_next_ = true;
    bool background_playback_ = true;
    float volume_ = 0.8f;

    std::string theme_ = "dark";
    std::string font_size_ = "medium";

    std::string download_path_ = "/switch/switch-audiobook/downloads/";
    int max_concurrent_downloads_ = 3;
    bool wifi_only_download_ = true;

    int http_timeout_ = 15000;
    bool verify_ssl_ = true;

    bool allow_analytics_ = false;

    void load_from_json();
    void write_to_json();
};