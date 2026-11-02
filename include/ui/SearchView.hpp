#pragma once

#include <borealis.hpp>
#include <vector>
#include <string>
#include <bilibili/Types.hpp>

class SearchView : public brls::Box {
public:
    SearchView(std::shared_ptr<class bilibili::BiliBiliClient> bili_client,
               std::shared_ptr<class AudioPlayer> audio_player);
    ~SearchView() = default;

    using SearchCallback = std::function<void(const std::string&)>;
    using PlayCallback = std::function<void(const bilibili::SearchResult&)>;

    void set_search_callback(SearchCallback cb) { search_cb_ = std::move(cb); }
    void set_play_callback(PlayCallback cb) { play_cb_ = std::move(cb); }

    void search(const std::string& query);
    void clear_results();

private:
    std::shared_ptr<bilibili::BiliBiliClient> bili_client_;
    std::shared_ptr<AudioPlayer> audio_player_;

    brls::TextInput* search_input_ = nullptr;
    brls::Button* search_btn_ = nullptr;
    brls::RecyclerView* results_list_ = nullptr;
    brls::ProgressBar* loading_indicator_ = nullptr;
    brls::Label* empty_label_ = nullptr;

    std::vector<bilibili::SearchResult> results_;
    std::string last_query_;
    int current_page_ = 1;
    bool is_loading_ = false;
    bool has_more_ = true;

    SearchCallback search_cb_;
    std::function<void(const bilibili::SearchResult&)> play_cb_;

    void setup_ui();
    void perform_search(const std::string& query, bool append = false);
    void on_search_completed(const std::vector<bilibili::SearchResult>& results, bool append);
    void on_search_error(const std::string& error);

    class ResultCell : public brls::Box {
    public:
        ResultCell(const bilibili::SearchResult& result);
        void set_on_click(std::function<void()> cb);
    private:
        brls::Image* cover_ = nullptr;
        brls::Label* title_ = nullptr;
        brls::Label* author_ = nullptr;
        brls::Label* duration_ = nullptr;
        brls::Label* badge_ = nullptr;
    };

    class SearchAdapter : public brls::RecyclerViewAdapter {
    public:
        SearchAdapter(const std::vector<bilibili::SearchResult>& results,
                      std::function<void(const bilibili::SearchResult&)> on_click);
        size_t getItemCount() override;
        std::unique_ptr<brls::ViewHolder> onCreateViewHolder(brls::View* parent) override;
        void onBindViewHolder(std::unique_ptr<brls::ViewHolder>& holder, size_t position) override;
    private:
        const std::vector<bilibili::SearchResult>& results_;
        std::function<void(const bilibili::SearchResult&)> on_click_;
    };
};