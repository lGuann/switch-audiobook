#include "ui/SearchView.hpp"
#include "bilibili/BiliBiliClient.hpp"
#include "player/AudioPlayer.hpp"
#include "utils/Logger.hpp"
#include <borealis.hpp>

using namespace brls;

SearchView::SearchView(std::shared_ptr<bilibili::BiliBiliClient> bili_client,
                       std::shared_ptr<AudioPlayer> audio_player)
    : bili_client_(std::move(bili_client)), audio_player_(std::move(audio_player)) {
    setup_ui();
}

void SearchView::setup_ui() {
    setDirection(Axis::Column);
    setPadding(16);

    // Search input
    search_input_ = new TextInput();
    search_input_->setPlaceholder("搜索有声书、播客、音频...");
    search_input_->setHint("搜索有声书、播客、音频...");
    search_input_->setOnSubmit([this](const std::string& text) {
        if (!text.empty()) {
            perform_search(text);
            if (search_cb_) search_cb_(text);
        }
    });
    addView(search_input_);

    // Search button
    search_btn_ = new Button("搜索");
    search_btn_->setOnClick([this] {
        std::string query = search_input_->getText();
        if (!query.empty()) {
            perform_search(query);
            if (search_cb_) search_cb_(query);
        }
    });
    addView(search_btn_);

    // Loading indicator
    loading_indicator_ = new ProgressBar();
    loading_indicator_->setIndeterminate(true);
    loading_indicator_->setVisibility(Visibility::Gone);
    addView(loading_indicator_);

    // Empty label
    empty_label_ = new Label("搜索有声书、播客、音频内容...");
    empty_label_->setHorizontalAlign(NVG_ALIGN_CENTER);
    empty_label_->setVisibility(Visibility::Visible);
    addView(empty_label_);

    // Results list
    results_list_ = new RecyclerView();
    results_list_->setVisibility(Visibility::Gone);
    addView(results_list_);

    // Load more button
    Button* load_more_btn = new Button("加载更多");
    load_more_btn->setVisibility(Visibility::Gone);
    load_more_btn->setOnClick([this] {
        if (!is_loading_ && has_more_) {
            current_page_++;
            perform_search(last_query_, true);
        }
    });
    addView(load_more_btn);
}

void SearchView::search(const std::string& query) {
    last_query_ = query;
    current_page_ = 1;
    perform_search(query);
}

void SearchView::clear_results() {
    results_.clear();
    last_query_.clear();
    current_page_ = 1;
    has_more_ = true;
    results_list_->setVisibility(Visibility::Gone);
    empty_label_->setVisibility(Visibility::Visible);
}

void SearchView::perform_search(const std::string& query, bool append) {
    if (is_loading_) return;

    is_loading_ = true;
    loading_indicator_->setVisibility(Visibility::Visible);
    empty_label_->setVisibility(Visibility::Gone);

    if (!append) {
        results_.clear();
        results_list_->setVisibility(Visibility::Gone);
    }

    auto results = bili_client_->search_audiobooks(query, current_page_);

    is_loading_ = false;
    loading_indicator_->setVisibility(Visibility::Gone);

    on_search_completed(results, append);
}

void SearchView::on_search_completed(const std::vector<bilibili::SearchResult>& new_results, bool append) {
    if (!append) {
        results_ = new_results;
    } else {
        results_.insert(results_.end(), new_results.begin(), new_results.end());
    }

    has_more_ = new_results.size() >= 30;

    if (results_.empty()) {
        empty_label_->setText("未找到相关内容");
        empty_label_->setVisibility(Visibility::Visible);
        results_list_->setVisibility(Visibility::Gone);
    } else {
        empty_label_->setVisibility(Visibility::Gone);
        results_list_->setVisibility(Visibility::Visible);

        auto adapter = std::make_shared<SearchAdapter>(results_, [this](const bilibili::SearchResult& item) {
            if (play_cb_) play_cb_(item);
        });
        results_list_->setAdapter(adapter);
    }
}

void SearchView::on_search_error(const std::string& error) {
    is_loading_ = false;
    loading_indicator_->setVisibility(Visibility::Gone);
    empty_label_->setText("搜索失败: " + error);
    empty_label_->setVisibility(Visibility::Visible);
}

// ResultCell implementation
SearchView::ResultCell::ResultCell(const bilibili::SearchResult& result) {
    setDirection(Axis::Row);
    setPadding(12);
    setHeight(80);

    cover_ = new Image();
    cover_->setSize(60, 60);
    if (!result.cover.empty()) {
        cover_->setImageFromURL(result.cover);
    }
    addView(cover_);

    Box* info_box = new Box();
    info_box->setDirection(Axis::Column);
    info_box->setGrow(1);

    title_ = new Label(result.title);
    title_->setFontSize(18);
    title_->setMaxLines(1);
    info_box->addView(title_);

    author_ = new Label(result.author);
    author_->setFontSize(14);
    author_->setTextColor(RGB(180, 180, 180));
    info_box->addView(author_);

    Box* meta_box = new Box();
    meta_box->setDirection(Axis::Row);

    duration_ = new Label("");
    if (result.duration > 0) {
        int min = result.duration / 60;
        int sec = result.duration % 60;
        duration_->setText(std::to_string(min) + ":" + (sec < 10 ? "0" : "") + std::to_string(sec));
    }
    meta_box->addView(duration_);

    if (result.is_audio) {
        badge_ = new Label("音频");
        badge_->setFontSize(12);
        badge_->setTextColor(RGB(0, 180, 255));
        meta_box->addView(badge_);
    }

    info_box->addView(meta_box);
    addView(info_box);
}

void SearchView::ResultCell::set_on_click(std::function<void()> cb) {
    setOnClick(cb);
}

// SearchAdapter implementation
SearchView::SearchAdapter::SearchAdapter(const std::vector<bilibili::SearchResult>& results,
                                         std::function<void(const bilibili::SearchResult&)> on_click)
    : results_(results), on_click_(std::move(on_click)) {}

size_t SearchView::SearchAdapter::getItemCount() {
    return results_.size();
}

std::unique_ptr<ViewHolder> SearchView::SearchAdapter::onCreateViewHolder(View* parent) {
    auto holder = std::make_unique<ViewHolder>(parent);
    holder->view = new ResultCell(results_[0]); // Will be replaced in onBind
    return holder;
}

void SearchView::SearchAdapter::onBindViewHolder(std::unique_ptr<ViewHolder>& holder, size_t position) {
    if (position >= results_.size()) return;

    auto* cell = dynamic_cast<ResultCell*>(holder->view.get());
    if (!cell) {
        cell = new ResultCell(results_[position]);
        holder->view.reset(cell);
    } else {
        // Update cell with new data
        // Would need to add update method to ResultCell
    }

    cell->set_on_click([this, position]() {
        if (on_click_ && position < results_.size()) {
            on_click_(results_[position]);
        }
    });
}