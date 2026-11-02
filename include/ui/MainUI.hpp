#pragma once

#include <borealis.hpp>
#include <memory>
#include <bilibili/BiliBiliClient.hpp>
#include <player/AudioPlayer.hpp>
#include <core/Config.hpp>

class MainUI : public brls::Box {
public:
    MainUI(std::shared_ptr<bilibili::BiliBiliClient> bili_client,
           std::shared_ptr<AudioPlayer> audio_player,
           std::shared_ptr<Config> config);
    ~MainUI() = default;

private:
    std::shared_ptr<bilibili::BiliBiliClient> bili_client_;
    std::shared_ptr<AudioPlayer> audio_player_;
    std::shared_ptr<Config> config_;

    brls::TabFrame* tab_frame_ = nullptr;
    brls::NavigationSidebar* sidebar_ = nullptr;

    class SearchView* search_view_ = nullptr;
    class PlayerView* player_view_ = nullptr;
    class LibraryView* library_view_ = nullptr;
    class SettingsView* settings_view_ = nullptr;

    brls::Box* mini_player_ = nullptr;
    brls::Label* mini_title_ = nullptr;
    brls::Label* mini_author_ = nullptr;
    brls::ProgressBar* mini_progress_ = nullptr;
    brls::Image* mini_play_pause_ = nullptr;
    brls::Button* mini_next_ = nullptr;

    std::string current_search_query_;

    void setup_ui();
    void setup_sidebar();
    void setup_tab_frame();
    void setup_mini_player();
    void bind_audio_player();

    void on_tab_selected(int index);
    void on_search_submitted(const std::string& query);
    void on_play_item(const bilibili::SearchResult& item);
    void on_playlist_item(const bilibili::PlaylistItem& item, int index);

    void on_mini_play_pause();
    void on_mini_next();
    void on_mini_tap();

    void on_audio_state_changed(AudioPlayer::State state);
    void on_audio_progress(float current, float total);
    void on_audio_error(const std::string& error);
    void on_audio_metadata(const bilibili::AudioInfo& info);

    void update_sleep_timer_display();
};