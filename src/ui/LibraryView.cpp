#include "ui/LibraryView.hpp"
#include "bilibili/BiliBiliClient.hpp"
#include "player/AudioPlayer.hpp"
#include <borealis.hpp>

using namespace brls;

LibraryView::LibraryView(std::shared_ptr<bilibili::BiliBiliClient> bili_client,
                         std::shared_ptr<AudioPlayer> audio_player)
    : bili_client_(std::move(bili_client)), audio_player_(std::move(audio_player)) {
    setup_ui();
}

void LibraryView::setup_ui() {
    setDirection(Axis::Column);
    setPadding(16);

    tab_frame_ = new TabFrame();

    // History tab
    Box* history_tab = new Box();
    history_tab->setDirection(Axis::Column);
    Label* history_label = new Label("播放历史");
    history_label->setFontSize(18);
    history_label->setMarginBottom(8);
    history_tab->addView(history_label);
    Label* history_empty = new Label("暂无播放历史");
    history_empty->setHorizontalAlign(NVG_ALIGN_CENTER);
    history_empty->setTextColor(RGB(180, 180, 180));
    history_empty->setMarginTop(48);
    history_tab->addView(history_empty);

    // Favorites tab
    Box* favorites_tab = new Box();
    favorites_tab->setDirection(Axis::Column);
    Label* favorites_label = new Label("我的收藏");
    favorites_label->setFontSize(18);
    favorites_label->setMarginBottom(8);
    favorites_tab->addView(favorites_label);
    Label* favorites_empty = new Label("暂无收藏");
    favorites_empty->setHorizontalAlign(NVG_ALIGN_CENTER);
    favorites_empty->setTextColor(RGB(180, 180, 180));
    favorites_empty->setMarginTop(48);
    favorites_tab->addView(favorites_empty);

    // Downloads tab
    Box* downloads_tab = new Box();
    downloads_tab->setDirection(Axis::Column);
    Label* downloads_label = new Label("下载管理");
    downloads_label->setFontSize(18);
    downloads_label->setMarginBottom(8);
    downloads_tab->addView(downloads_label);
    Label* downloads_empty = new Label("暂无下载");
    downloads_empty->setHorizontalAlign(NVG_ALIGN_CENTER);
    downloads_empty->setTextColor(RGB(180, 180, 180));
    downloads_empty->setMarginTop(48);
    downloads_tab->addView(downloads_empty);

    tab_frame_->addTab("历史", history_tab);
    tab_frame_->addTab("收藏", favorites_tab);
    tab_frame_->addTab("下载", downloads_tab);

    addView(tab_frame_);
}

void LibraryView::load_history() {
    // Load from local database
}

void LibraryView::load_favorites() {
    // Load from local database
}

void LibraryView::load_downloads() {
    // Load from local database
}