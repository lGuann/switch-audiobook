#pragma once

#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <memory>
#include <bilibili/Types.hpp>

namespace bilibili {

class HttpClient;
class Config;

class BiliBiliClient {
public:
    explicit BiliBiliClient(std::shared_ptr<HttpClient> http, std::shared_ptr<Config> config);
    ~BiliBiliClient() = default;

    BiliBiliClient(const BiliBiliClient&) = delete;
    BiliBiliClient& operator=(const BiliBiliClient&) = delete;
    BiliBiliClient(BiliBiliClient&&) = default;
    BiliBiliClient& operator=(BiliBiliClient&&) = default;

    bool login(const std::string& sessdata, const std::string& bili_jct = "", const std::string& buvid3 = "");
    void logout();
    bool is_logged_in() const;
    void refresh_cookies();

    std::vector<SearchResult> search_audiobooks(const std::string& keyword, int page = 1, int page_size = 30);
    std::vector<SearchResult> search_videos(const std::string& keyword, int page = 1, int page_size = 30, bool audio_only = true);
    std::vector<SearchResult> search_up_audiobooks(int mid, int page = 1, int page_size = 30);

    std::optional<AudioInfo> get_audio_info(const std::string& bvid);
    std::optional<AudioInfo> get_audio_info_by_aid(int64_t aid);
    std::vector<PlaylistItem> get_playlist(const std::string& bvid);

    std::vector<AudioStream> get_audio_streams(const std::string& bvid, const std::string& cid);
    std::vector<AudioStream> get_audio_streams_by_aid(int64_t aid, const std::string& cid);
    std::optional<AudioStream> get_best_audio_stream(const std::string& bvid, const std::string& cid);

    std::optional<SeasonInfo> get_season_info(const std::string& season_id);
    std::vector<PlaylistItem> get_season_episodes(const std::string& season_id, int page = 1, int page_size = 30);

    std::optional<UserInfo> get_user_info(int mid);
    std::vector<SearchResult> get_user_favorites(int mid, int page = 1);

    std::vector<SearchResult> get_hot_audiobooks(int page = 1, int page_size = 20);
    std::vector<SearchResult> get_new_audiobooks(int page = 1, int page_size = 20);

    void set_progress_callback(std::function<void(float)> cb);
    void set_error_callback(std::function<void(const std::string&)> cb);

private:
    std::shared_ptr<HttpClient> http_;
    std::shared_ptr<Config> config_;

    std::string sessdata_;
    std::string bili_jct_;
    std::string buvid3_;
    std::string wbi_img_url_;
    std::string wbi_sub_url_;

    std::function<void(float)> progress_cb_;
    std::function<void(const std::string&)> error_cb_;

    std::string build_url(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params) const;
    std::string sign_wbi(const std::string& url, const std::vector<std::pair<std::string, std::string>>& params) const;
    void update_wbi_keys();
    nlohmann::json request(const std::string& method, const std::string& url, const std::string& body = "");
    void handle_rate_limit(const nlohmann::json& response);

    std::vector<SearchResult> parse_search_results(const nlohmann::json& data);
    std::vector<PlaylistItem> parse_playlist(const nlohmann::json& data);
    std::vector<AudioStream> parse_audio_streams(const nlohmann::json& data);
};

} // namespace bilibili