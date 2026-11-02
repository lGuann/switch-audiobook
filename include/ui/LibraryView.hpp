#pragma once

#include <borealis.hpp>
#include <memory>
#include <bilibili/BiliBiliClient.hpp>
#include <player/AudioPlayer.hpp>

class LibraryView : public brls::Box {
public:
    LibraryView(std::shared_ptr<bilibili::BiliBiliClient> bili_client,
                std::shared_ptr<AudioPlayer> audio_player);
    ~LibraryView() = default;

private:
    std::shared_ptr<bilibili::BiliBiliClient> bili_client_;
    std::shared_ptr<AudioPlayer> audio_player_;

    brls::TabFrame* tab_frame_ = nullptr;

    void setup_ui();
    void load_history();
    void load_favorites();
    void load_downloads();
};