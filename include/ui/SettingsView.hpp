#pragma once

#include <borealis.hpp>
#include <memory>
#include <core/Config.hpp>

class SettingsView : public brls::Box {
public:
    SettingsView(std::shared_ptr<Config> config);
    ~SettingsView() = default;

private:
    std::shared_ptr<Config> config_;

    brls::TextInput* sessdata_input_ = nullptr;
    brls::TextInput* bili_jct_input_ = nullptr;
    brls::TextInput* buvid3_input_ = nullptr;
    brls::Button* login_btn_ = nullptr;
    brls::Button* logout_btn_ = nullptr;

    brls::Slider* volume_slider_ = nullptr;
    brls::Slider* speed_slider_ = nullptr;
    brls::Slider* sleep_slider_ = nullptr;

    brls::Switch* auto_play_switch_ = nullptr;
    brls::Switch* background_switch_ = nullptr;
    brls::Switch* wifi_download_switch_ = nullptr;

    brls::Dropdown* theme_dropdown_ = nullptr;
    brls::Dropdown* font_size_dropdown_ = nullptr;

    void setup_ui();
    void load_current_settings();
    void on_login();
    void on_logout();
    void save_settings();
};