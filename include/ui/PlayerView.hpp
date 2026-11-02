#pragma once

#include <borealis.hpp>
#include <memory>
#include <player/AudioPlayer.hpp>

class PlayerView : public brls::Box {
public:
    PlayerView(std::shared_ptr<AudioPlayer> audio_player);
    ~PlayerView() = default;

    void update_progress(float current, float total);
    void update_metadata(const bilibili::AudioInfo& info);

private:
    std::shared_ptr<AudioPlayer> audio_player_;

    brls::Image* cover_ = nullptr;
    brls::Label* title_ = nullptr;
    brls::Label* author_ = nullptr;
    brls::ProgressBar* progress_ = nullptr;
    brls::Label* current_time_ = nullptr;
    brls::Label* total_time_ = nullptr;
    brls::Button* play_pause_ = nullptr;
    brls::Button* prev_ = nullptr;
    brls::Button* next_ = nullptr;
    brls::Button* speed_ = nullptr;
    brls::Button* sleep_ = nullptr;
    brls::Slider* volume_ = nullptr;

    void setup_ui();
    void bind_player();
    void update_play_pause_icon();
    void format_time(int64_t ms, std::string& out);
};