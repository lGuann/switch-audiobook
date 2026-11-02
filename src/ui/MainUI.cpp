#include "ui/MainUI.hpp"
#include "ui/SearchView.hpp"
#include "ui/PlayerView.hpp"
#include "ui/SettingsView.hpp"
#include "player/AudioPlayer.hpp"
#include "bilibili/BiliBiliClient.hpp"
#include "core/Config.hpp"
#include "utils/Logger.hpp"
#include <borealis.hpp>

using namespace brls;

MainUI::MainUI(std::shared_ptr<bilibili::BiliBiliClient> bili_client,
               std::shared_ptr<AudioPlayer> audio_player,
               std::shared_ptr<Config> config)
    : bili_client_(std::move(bili_client)),
      audio_player_(std::move(audio_player)),
      config_(std::move(config)) {
    setup_ui();
    setup_sidebar();
    setup_tab_frame();
    setup_mini_player();
    bind_audio_player();
}

void MainUI::setup_ui() {
    set_padding(0);
}

void MainUI::setup_sidebar() {
    sidebar_ = new NavigationSidebar();
    sidebar_->addItem("搜索", "search", [this] { on_tab_selected(0); });
    sidebar_->addItem("播放器", "play", [this] { on_tab_selected(1); });
    sidebar_->addItem("我的", "library", [this] { on_tab_selected(2); });
    sidebar_->addItem("设置", "settings", [this] { on_tab_selected(3); });
    sidebar_->setSelectedItem(0);

    addView(sidebar_);
}

void MainUI::setup_tab_frame() {
    tab_frame_ = new TabFrame();

    search_view_ = new SearchView(bili_client_, audio_player_);
    search_view_->set_search_callback([this](const std::string& query) {
        current_search_query_ = query;
    });
    search_view_->set_play_callback([this](const bilibili::SearchResult& item) {
        on_play_item(item);
    });

    player_view_ = new PlayerView(audio_player_);
    library_view_ = new LibraryView(bili_client_, audio_player_);
    settings_view_ = new SettingsView(config_);

    tab_frame_->addTab("搜索", search_view_);
    tab_frame_->addTab("播放器", player_view_);
    tab_frame_->addTab("我的", library_view_);
    tab_frame_->addTab("设置", settings_view_);

    addView(tab_frame_);
}

void MainUI::setup_mini_player() {
    mini_player_ = new Box();
    mini_player_->setVisibility(Visibility::Gone);
    mini_player_->setDirection(Axis::Row);
    mini_player_->setPadding(12);
    mini_player_->setBackgroundColor(RGB(30, 30, 30, 240));

    mini_play_pause_ = new Image();
    mini_play_pause_->setImageFromRes("play.png");
    mini_play_pause_->setSize(40, 40);

    Box* info_box = new Box();
    info_box->setDirection(Axis::Column);
    info_box->setGrow(1);

    mini_title_ = new Label("标题");
    mini_title_->setFontSize(18);
    mini_title_->setTextColor(RGB(255, 255, 255));

    mini_author_ = new Label("作者");
    mini_author_->setFontSize(14);
    mini_author_->setTextColor(RGB(180, 180, 180));

    info_box->addView(mini_title_);
    info_box->addView(mini_author_);

    mini_progress_ = new ProgressBar();
    mini_progress_->setProgress(0);
    mini_progress_->setHeight(4);

    mini_next_ = new Button("下一曲");
    mini_next_->setOnClick([this] { on_mini_next(); });

    mini_player_->addView(mini_play_pause_);
    mini_player_->addView(info_box);
    mini_player_->addView(mini_progress_);
    mini_player_->addView(mini_next_);

    addView(mini_player_);

    // Make mini player clickable
    mini_player_->setOnClick([this] { on_mini_tap(); });
}

void MainUI::bind_audio_player() {
    audio_player_->set_state_callback([this](AudioPlayer::State state) {
        on_audio_state_changed(state);
    });
    audio_player_->set_progress_callback([this](float current, float total) {
        on_audio_progress(current, total);
    });
    audio_player_->set_error_callback([this](const std::string& error) {
        on_audio_error(error);
    });
    audio_player_->set_metadata_callback([this](const bilibili::AudioInfo& info) {
        on_audio_metadata(info);
    });

    audio_player_->set_sleep_callback([this]() {
        // Sleep timer triggered
    });
}

void MainUI::setup_mini_player() {
    // Already set up in setup_mini_player
}

void MainUI::on_tab_selected(int index) {
    if (tab_frame_) {
        tab_frame_->setCurrentTab(index);
    }
    if (sidebar_) {
        sidebar_->setSelectedItem(index);
    }
}

void MainUI::on_search_submitted(const std::string& query) {
    if (search_view_) {
        search_view_->search(query);
    }
    on_tab_selected(0);
}

void MainUI::on_play_item(const bilibili::SearchResult& item) {
    auto streams = bili_client_->get_audio_streams(item.bvid, "");
    if (!streams.empty()) {
        audio_player_->play(streams[0], bilibili::AudioInfo{
            .bvid = item.bvid,
            .title = item.title,
            .author = item.author,
            .cover = item.cover,
            .duration = item.duration,
        });
        show_mini_player(item.title, item.author);
    }
}

void MainUI::on_playlist_item(const bilibili::PlaylistItem& item, int index) {
    // Play playlist item
}

void MainUI::on_mini_play_pause() {
    if (audio_player_->get_state() == AudioPlayer::State::Playing) {
        audio_player_->pause();
    } else {
        audio_player_->resume();
    }
}

void MainUI::on_mini_next() {
    audio_player_->play_next();
}

void MainUI::on_mini_tap() {
    on_tab_selected(1); // Switch to player view
}

void MainUI::on_audio_state_changed(AudioPlayer::State state) {
    if (mini_play_pause_) {
        if (state == AudioPlayer::State::Playing) {
            mini_play_pause_->setImageFromRes("pause.png");
        } else {
            mini_play_pause_->setImageFromRes("play.png");
        }
    }

    if (state == AudioPlayer::State::Playing) {
        mini_player_->setVisibility(Visibility::Visible);
    } else if (state == AudioPlayer::State::Stopped || state == AudioPlayer::State::Ended) {
        // Keep mini player visible but show paused state
    }
}

void MainUI::on_audio_progress(float current, float total) {
    if (mini_progress_) {
        float progress = (total > 0) ? (current / total) : 0;
        mini_progress_->setProgress(progress);
    }
    if (player_view_) {
        player_view_->update_progress(current, total);
    }
}

void MainUI::on_audio_error(const std::string& error) {
    Logger::error("Audio error: {}", error);
}

void MainUI::on_audio_metadata(const bilibili::AudioInfo& info) {
    if (mini_title_) mini_title_->setText(info.title);
    if (mini_author_) mini_author_->setText(info.author);
    if (player_view_) {
        player_view_->update_metadata(info);
    }
}

void MainUI::show_mini_player(const std::string& title, const std::string& author) {
    if (mini_title_) mini_title_->setText(title);
    if (mini_author_) mini_author_->setText(author);
    if (mini_player_) mini_player_->setVisibility(Visibility::Visible);
}

void MainUI::update_sleep_timer_display() {
    // Update sleep timer UI
}