#include "ui/SettingsView.hpp"
#include "core/Config.hpp"
#include <borealis.hpp>

using namespace brls;

SettingsView::SettingsView(std::shared_ptr<Config> config)
    : config_(std::move(config)) {
    setup_ui();
    load_current_settings();
}

void SettingsView::setup_ui() {
    setDirection(Axis::Column);
    setPadding(24);

    // Account section
    Label* account_label = new Label("账号设置");
    account_label->setFontSize(20);
    addView(account_label);

    sessdata_input_ = new TextInput();
    sessdata_input_->setPlaceholder("SESSDATA");
    addView(sessdata_input_);

    bili_jct_input_ = new TextInput();
    bili_jct_input_->setPlaceholder("bili_jct");
    addView(bili_jct_input_);

    buvid3_input_ = new TextInput();
    buvid3_input_->setPlaceholder("buvid3");
    addView(buvid3_input_);

    Box* btn_box = new Box();
    btn_box->setDirection(Axis::Row);
    btn_box->setMarginTop(8);

    login_btn_ = new Button("登录");
    login_btn_->setOnClick([this] { on_login(); });
    addView(login_btn_);

    logout_btn_ = new Button("登出");
    logout_btn_->setOnClick([this] { on_logout(); });
    addView(logout_btn_);

    // Playback settings
    Label* playback_label = new Label("播放设置");
    playback_label->setFontSize(20);
    playback_label->setMarginTop(24);
    addView(playback_label);

    // Volume
    Box* vol_box = new Box();
    vol_box->setDirection(Axis::Row);
    vol_box->setMarginTop(8);

    Label* vol_label = new Label("音量");
    vol_label->setMinWidth(80);
    addView(vol_label);

    volume_slider_ = new Slider();
    volume_slider_->setGrow(1);
    volume_slider_->setProgress(0.8f);
    volume_slider_->setOnProgressChange([this](float value) {
        config_->set_volume(value);
    });
    addView(volume_slider_);

    // Speed
    Box* speed_box = new Box();
    speed_box->setDirection(Axis::Row);
    speed_box->setMarginTop(8);

    Label* speed_label = new Label("倍速");
    speed_label->setMinWidth(80);
    addView(speed_label);

    speed_slider_ = new Slider();
    speed_slider_->setMin(0.5f);
    speed_slider_->setMax(3.0f);
    speed_slider_->setGrow(1);
    speed_slider_->setProgress(1.0f);
    speed_slider_->setOnProgressChange([this](float value) {
        config_->set_playback_speed(value);
    });
    addView(speed_slider_);

    // Sleep timer
    Box* sleep_box = new Box();
    sleep_box->setDirection(Axis::Row);
    sleep_box->setMarginTop(8);

    Label* sleep_label = new Label("睡眠定时(分钟)");
    sleep_label->setMinWidth(120);
    addView(sleep_label);

    sleep_slider_ = new Slider();
    sleep_slider_->setMin(0);
    sleep_slider_->setMax(120);
    sleep_slider_->setGrow(1);
    sleep_slider_->setProgress(0);
    sleep_slider_->setOnProgressChange([this](float value) {
        // Would need to update audio player sleep timer
    });
    addView(sleep_slider_);

    // Toggles
    Label* toggles_label = new Label("其他设置");
    toggles_label->setFontSize(20);
    toggles_label->setMarginTop(24);
    addView(toggles_label);

    auto_play_switch_ = new Switch("自动播放下一曲", config_->get_auto_play_next());
    auto_play_switch_->setOnValueChange([this](bool value) {
        config_->set_auto_play_next(value);
    });
    addView(auto_play_switch_);

    background_switch_ = new Switch("后台播放", config_->get_background_playback());
    background_switch_->setOnValueChange([this](bool value) {
        config_->set_background_playback(value);
    });
    addView(background_switch_);

    wifi_download_switch_ = new Switch("仅WiFi下载", config_->get_wifi_only_download());
    wifi_download_switch_->setOnValueChange([this](bool value) {
        config_->set_wifi_only_download(value);
    });
    addView(wifi_download_switch_);

    // Theme
    Box* theme_box = new Box();
    theme_box->setDirection(Axis::Row);
    theme_box->setMarginTop(8);

    Label* theme_label = new Label("主题");
    theme_label->setMinWidth(80);
    addView(theme_label);

    theme_dropdown_ = new Dropdown();
    theme_dropdown_->addItem("深色", "dark");
    theme_dropdown_->addItem("浅色", "light");
    theme_dropdown_->addItem("跟随系统", "system");
    theme_dropdown_->setSelectedItem(config_->get_theme());
    theme_dropdown_->setOnItemSelected([this](const std::string& value) {
        config_->set_theme(value);
    });
    addView(theme_dropdown_);

    // Font size
    Box* font_box = new Box();
    font_box->setDirection(Axis::Row);
    font_box->setMarginTop(8);

    Label* font_label = new Label("字体大小");
    font_label->setMinWidth(80);
    addView(font_label);

    font_size_dropdown_ = new Dropdown();
    font_size_dropdown_->addItem("小", "small");
    font_size_dropdown_->addItem("中", "medium");
    font_size_dropdown_->addItem("大", "large");
    font_size_dropdown_->setSelectedItem(config_->get_font_size());
    font_size_dropdown_->setOnItemSelected([this](const std::string& value) {
        config_->set_font_size(value);
    });
    addView(font_size_dropdown_);

    // Save button
    Button* save_btn = new Button("保存设置");
    save_btn->setMarginTop(24);
    save_btn->setOnClick([this] { save_settings(); });
    addView(save_btn);
}

void SettingsView::load_current_settings() {
    if (config_->has_valid_cookies()) {
        sessdata_input_->setText(config_->get_sessdata());
        bili_jct_input_->setText(config_->get_bili_jct());
        buvid3_input_->setText(config_->get_buvid3());
    }

    volume_slider_->setProgress(config_->get_volume());
    speed_slider_->setProgress(config_->get_playback_speed());
}

void SettingsView::on_login() {
    // Would trigger login flow
}

void SettingsView::on_logout() {
    // Would trigger logout
}

void SettingsView::save_settings() {
    config_->save();
}