#include "ui/PlayerView.hpp"
#include "player/AudioPlayer.hpp"
#include <borealis.hpp>
#include <iomanip>
#include <sstream>

using namespace brls;

PlayerView::PlayerView(std::shared_ptr<AudioPlayer> audio_player)
    : audio_player_(std::move(audio_player)) {
    setup_ui();
    bind_player();
}

void PlayerView::setup_ui() {
    setDirection(Axis::Column);
    setPadding(24);
    setAlignItems(NVG_ALIGN_CENTER);

    // Cover art
    cover_ = new Image();
    cover_->setSize(300, 300);
    cover_->setCornerRadius(12);
    addView(cover_);

    // Title
    title_ = new Label("标题");
    title_->setFontSize(24);
    title_->setHorizontalAlign(NVG_ALIGN_CENTER);
    title_->setMaxLines(2);
    addView(title_);

    // Author
    author_ = new Label("作者");
    author_->setFontSize(18);
    author_->setTextColor(RGB(180, 180, 180));
    author_->setHorizontalAlign(NVG_ALIGN_CENTER);
    addView(author_);

    // Progress
    Box* progress_box = new Box();
    progress_box->setDirection(Axis::Row);
    progress_box->setMarginTop(24);
    progress_box->setGrow(1);

    current_time_ = new Label("0:00");
    current_time_->setFontSize(14);
    progress_box->addView(current_time_);

    progress_ = new ProgressBar();
    progress_->setGrow(1);
    progress_->setMarginLeft(12);
    progress_box->addView(progress_);

    total_time_ = new Label("0:00");
    total_time_->setFontSize(14);
    progress_box->addView(total_time_);

    addView(progress_box);

    // Controls
    Box* controls_box = new Box();
    controls_box->setDirection(Axis::Row);
    controls_box->setMarginTop(24);
    controls_box->setAlignItems(NVG_ALIGN_CENTER);

    prev_ = new Button("⏮");
    prev_->setOnClick([this] { audio_player_->play_prev(); });
    controls_box->addView(prev_);

    play_pause_ = new Button("⏸");
    play_pause_->setOnClick([this] { audio_player_->toggle_pause(); });
    controls_box->addView(play_pause_);

    next_ = new Button("⏭");
    next_->setOnClick([this] { audio_player_->play_next(); });
    controls_box->addView(next_);

    speed_ = new Button("1.0x");
    speed_->setOnClick([this] {
        float speeds[] = {1.0f, 1.25f, 1.5f, 2.0f, 0.5f, 0.75f};
        float current = audio_player_->get_speed();
        float next = speeds[0];
        for (size_t i = 0; i < 6; ++i) {
            if (speeds[i] > current + 0.01f) {
                next = speeds[i];
                break;
            }
        }
        audio_player_->set_speed(next);
    });
    controls_box->addView(speed_);

    sleep_ = new Button("🕐");
    sleep_->setOnClick([this] {
        // Show sleep timer dialog
    });
    controls_box->addView(sleep_);

    addView(controls_box);

    // Volume
    Box* volume_box = new Box();
    volume_box->setDirection(Axis::Row);
    volume_box->setMarginTop(16);
    volume_box->setAlignItems(NVG_ALIGN_CENTER);

    Label* vol_label = new Label("音量");
    vol_label->setMarginRight(12);
    volume_box->addView(vol_label);

    volume_ = new Slider();
    volume_->setGrow(1);
    volume_->setProgress(0.8f);
    volume_->setOnProgressChange([this](float value) {
        audio_player_->set_volume(value);
    });
    volume_box->addView(volume_);

    addView(volume_box);
}

void PlayerView::bind_player() {
    audio_player_->set_state_callback([this](AudioPlayer::State state) {
        if (state == AudioPlayer::State::Playing) {
            play_pause_->setText("⏸");
        } else {
            play_pause_->setText("▶");
        }
    });

    audio_player_->set_progress_callback([this](float current, float total) {
        update_progress(current, total);
    });

    audio_player_->set_metadata_callback([this](const bilibili::AudioInfo& info) {
        update_metadata(info);
    });
}

void PlayerView::update_progress(float current, float total) {
    if (progress_) {
        float progress = (total > 0) ? (current / total) : 0.0f;
        progress_->setProgress(progress);
    }

    std::string current_str, total_str;
    format_time(static_cast<int64_t>(current * 1000), current_str);
    format_time(static_cast<int64_t>(total * 1000), total_str);

    if (current_time_) current_time_->setText(current_str);
    if (total_time_) total_time_->setText(total_str);
}

void PlayerView::update_metadata(const bilibili::AudioInfo& info) {
    if (title_) title_->setText(info.title);
    if (author_) author_->setText(info.author);
    if (cover_ && !info.cover.empty()) {
        cover_->setImageFromURL(info.cover);
    }
}

void PlayerView::format_time(int64_t ms, std::string& out) {
    int total_sec = ms / 1000;
    int min = total_sec / 60;
    int sec = total_sec % 60;
    std::ostringstream oss;
    oss << min << ":" << std::setw(2) << std::setfill('0') << sec;
    out = oss.str();
}