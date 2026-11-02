#include "bilibili/BiliBiliClient.hpp"
#include "bilibili/Types.hpp"
#include "core/Config.hpp"
#include "network/HttpClient.hpp"
#include "utils/Logger.hpp"
#include <nlohmann/json.hpp>
#include <openssl/md5.h>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

namespace bilibili {

namespace {
const std::string BASE_URL = "https://api.bilibili.com";
const std::string AUDIO_BASE_URL = "https://www.bilibili.com/audio/music-service-c";

std::string url_encode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int((unsigned char)c);
        }
    }
    return escaped.str();
}

std::string md5_hash(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((const unsigned char*)input.c_str(), input.length(), digest);
    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    }
    return oss.str();
}

std::string get_wbi_sign(const std::string& url, const std::vector<std::pair<std::string, std::string>>& params,
                         const std::string& img_key, const std::string& sub_key) {
    std::vector<std::pair<std::string, std::string>> sorted_params = params;
    std::sort(sorted_params.begin(), sorted_params.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::string query;
    for (const auto& p : sorted_params) {
        if (!query.empty()) query += "&";
        query += p.first + "=" + url_encode(p.second);
    }

    std::string mix_key = img_key + sub_key;
    std::string sign_str = query + mix_key;
    return md5_hash(sign_str);
}
} // namespace

namespace bilibili {

BiliBiliClient::BiliBiliClient(std::shared_ptr<HttpClient> http, std::shared_ptr<Config> config)
    : http_(std::move(http)), config_(std::move(config)) {}

BiliBiliClient::~BiliBiliClient() = default;

bool BiliBiliClient::login(const std::string& sessdata, const std::string& bili_jct, const std::string& buvid3) {
    sessdata_ = sessdata;
    bili_jct_ = bili_jct;
    buvid3_ = buvid3;

    config_->set_cookies(sessdata_, bili_jct_, buvid3_);

    update_wbi_keys();
    return true;
}

void BiliBiliClient::logout() {
    sessdata_.clear();
    bili_jct_.clear();
    buvid3_.clear();
    wbi_img_url_.clear();
    wbi_sub_key_.clear();
    config_->clear_all_data();
}

bool BiliBiliClient::is_logged_in() const {
    return !sessdata_.empty();
}

void BiliBiliClient::refresh_cookies() {
    update_wbi_keys();
}

std::vector<SearchResult> BiliBiliClient::search_audiobooks(const std::string& keyword, int page, int page_size) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"keyword", keyword},
        {"page", std::to_string(page)},
        {"page_size", std::to_string(page_size)},
        {"search_type", "audio"},
        {"highlight", "1"},
    };

    std::string url = build_url("/x/web-interface/search/all/v2", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) {
        if (error_cb_) error_cb_("Search failed: " + response.error_message);
        return {};
    }

    return parse_search_results(response.body);
}

std::vector<SearchResult> BiliBiliClient::search_videos(const std::string& keyword, int page, int page_size, bool audio_only) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"keyword", keyword},
        {"page", std::to_string(page)},
        {"page_size", std::to_string(page_size)},
        {"search_type", audio_only ? "audio" : "video"},
    };

    std::string url = build_url("/x/web-interface/search/all/v2", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) {
        if (error_cb_) error_cb_("Search failed: " + response.error_message);
        return {};
    }

    return parse_search_results(response.body);
}

std::vector<SearchResult> BiliBiliClient::search_up_audiobooks(int mid, int page, int page_size) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"mid", std::to_string(mid)},
        {"pn", std::to_string(page)},
        {"ps", std::to_string(page_size)},
        {"order", "pubdate"},
        {"type", "audio"},
    };

    std::string url = build_url("/x/space/arc/search", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) {
        if (error_cb_) error_cb_("Search UP audiobooks failed: " + response.error_message);
        return {};
    }

    return parse_search_results(response.body);
}

std::optional<AudioInfo> BiliBiliClient::get_audio_info(const std::string& bvid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"bvid", bvid},
    };

    std::string url = build_url("/x/web-interface/view", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return std::nullopt;

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return std::nullopt;

    AudioInfo info;
    from_json(j["data"], info);
    return info;
}

std::optional<AudioInfo> BiliBiliClient::get_audio_info_by_aid(int64_t aid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"aid", std::to_string(aid)},
    };

    std::string url = build_url("/x/web-interface/view", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return std::nullopt;

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return std::nullopt;

    AudioInfo info;
    from_json(j["data"], info);
    return info;
}

std::vector<PlaylistItem> BiliBiliClient::get_playlist(const std::string& bvid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"bvid", bvid},
    };

    std::string url = build_url("/x/player/pagelist", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return {};

    return parse_playlist(j["data"]);
}

std::vector<AudioStream> BiliBiliClient::get_audio_streams(const std::string& bvid, const std::string& cid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"bvid", bvid},
        {"cid", cid},
        {"qn", "30280"},
        {"fnval", "16"},
        {"fnver", "4"},
        {"fourk", "1"},
    };

    std::string url = build_url("/x/player/playurl", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return {};

    return parse_audio_streams(j["data"]);
}

std::vector<AudioStream> BiliBiliClient::get_audio_streams_by_aid(int64_t aid, const std::string& cid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"aid", std::to_string(aid)},
        {"cid", cid},
        {"qn", "30280"},
        {"fnval", "16"},
        {"fnver", "4"},
        {"fourk", "1"},
    };

    std::string url = build_url("/x/player/playurl", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return {};

    return parse_audio_streams(j["data"]);
}

std::optional<AudioStream> BiliBiliClient::get_best_audio_stream(const std::string& bvid, const std::string& cid) {
    auto streams = get_audio_streams(bvid, cid);
    if (streams.empty()) return std::nullopt;

    std::sort(streams.begin(), streams.end(),
              [](const AudioStream& a, const AudioStream& b) {
                  return a.bitrate > b.bitrate;
              });

    return streams[0];
}

std::optional<SeasonInfo> BiliBiliClient::get_season_info(const std::string& season_id) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"season_id", season_id},
    };

    std::string url = build_url("/x/polymer/web-space/seasons_archives_list", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return std::nullopt;

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return std::nullopt;

    SeasonInfo season;
    from_json(j["data"], season);
    return season;
}

std::vector<PlaylistItem> BiliBiliClient::get_season_episodes(const std::string& season_id, int page, int page_size) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"season_id", season_id},
        {"page_num", std::to_string(page)},
        {"page_size", std::to_string(page_size)},
    };

    std::string url = build_url("/x/polymer/web-space/seasons_archives_list", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return {};

    return parse_playlist(j["data"]["archives"]);
}

std::optional<UserInfo> BiliBiliClient::get_user_info(int mid) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"mid", std::to_string(mid)},
    };

    std::string url = build_url("/x/space/acc/info", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return std::nullopt;

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] != 0) return std::nullopt;

    UserInfo user;
    from_json(j["data"], user);
    return user;
}

std::vector<SearchResult> BiliBiliClient::get_user_favorites(int mid, int page) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"mid", std::to_string(mid)},
        {"pn", std::to_string(page)},
        {"ps", "30"},
    };

    std::string url = build_url("/x/v3/fav/folder/created/list-all", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    return parse_search_results(response.body);
}

std::vector<SearchResult> BiliBiliClient::get_hot_audiobooks(int page, int page_size) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"page", std::to_string(page)},
        {"page_size", std::to_string(page_size)},
    };

    std::string url = build_url("/audio/music-service-c/web/rank/hot", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    return parse_search_results(response.body);
}

std::vector<SearchResult> BiliBiliClient::get_new_audiobooks(int page, int page_size) {
    std::vector<std::pair<std::string, std::string>> params = {
        {"page", std::to_string(page)},
        {"page_size", std::to_string(page_size)},
    };

    std::string url = build_url("/audio/music-service-c/web/rank/new", params);
    auto response = http_->get(url, get_auth_headers());

    if (!response.success) return {};

    return parse_search_results(response.body);
}

void BiliBiliClient::set_progress_callback(std::function<void(float)> cb) {
    progress_cb_ = std::move(cb);
}

void BiliBiliClient::set_error_callback(std::function<void(const std::string&)> cb) {
    error_cb_ = std::move(cb);
}

std::string BiliBiliClient::build_url(const std::string& path, const std::vector<std::pair<std::string, std::string>>& params) const {
    std::string url = BASE_URL + path;
    if (!params.empty()) {
        url += "?";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) url += "&";
            url += params[i].first + "=" + url_encode(params[i].second);
        }
    }
    return url;
}

std::string BiliBiliClient::sign_wbi(const std::string& url, const std::vector<std::pair<std::string, std::string>>& params) const {
    return get_wbi_sign(url, params, wbi_img_key_, wbi_sub_key_);
}

void BiliBiliClient::update_wbi_keys() {
    auto response = http_->get(BASE_URL + "/x/web-interface/nav", get_auth_headers());
    if (!response.success) return;

    nlohmann::json j = nlohmann::json::parse(response.body);
    if (j["code"] == 0 && j["data"].contains("wbi_img")) {
        wbi_img_url_ = j["data"]["wbi_img"]["img_url"].get<std::string>();
        wbi_sub_key_ = j["data"]["wbi_img"]["sub_url"].get<std::string>();

        size_t pos = wbi_img_url_.find_last_of('/');
        if (pos != std::string::npos) {
            wbi_img_key_ = wbi_img_url_.substr(pos + 1);
            pos = wbi_img_key_.find('.');
            if (pos != std::string::npos) wbi_img_key_ = wbi_img_key_.substr(0, pos);
        }

        pos = wbi_sub_key_.find_last_of('/');
        if (pos != std::string::npos) {
            wbi_sub_key_ = wbi_sub_key_.substr(pos + 1);
            pos = wbi_sub_key_.find('.');
            if (pos != std::string::npos) wbi_sub_key_ = wbi_sub_key_.substr(0, pos);
        }
    }
}

nlohmann::json BiliBiliClient::request(const std::string& method, const std::string& url, const std::string& body) {
    if (method == "GET") {
        return http_->get(url, get_auth_headers());
    } else {
        return http_->post(url, body, get_auth_headers());
    }
}

void BiliBiliClient::handle_rate_limit(const nlohmann::json& response) {
    if (response.contains("code") && response["code"] == -101) {
        // Rate limited, wait and retry
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::vector<std::pair<std::string, std::string>> BiliBiliClient::get_auth_headers() const {
    std::vector<std::pair<std::string, std::string>> headers;
    headers.emplace_back("Referer", "https://www.bilibili.com/");
    headers.emplace_back("Origin", "https://www.bilibili.com");
    headers.emplace_back("User-Agent", "Mozilla/5.0 (Nintendo Switch; libnx) SwitchAudiobook/0.1.0");

    if (!sessdata_.empty()) {
        std::string cookie = "SESSDATA=" + sessdata_;
        if (!bili_jct_.empty()) cookie += "; bili_jct=" + bili_jct_;
        if (!buvid3_.empty()) cookie += "; buvid3=" + buvid3_;
        cookie += "; buvid4=" + buvid3_; // fallback
        headers.emplace_back("Cookie", cookie);
    }
    return headers;
}

std::vector<SearchResult> BiliBiliClient::parse_search_results(const nlohmann::json& data) {
    std::vector<SearchResult> results;
    // Parse search results from JSON
    return results;
}

std::vector<PlaylistItem> BiliBiliClient::parse_playlist(const nlohmann::json& data) {
    std::vector<PlaylistItem> items;
    // Parse playlist from JSON
    return items;
}

std::vector<AudioStream> BiliBiliClient::parse_audio_streams(const nlohmann::json& data) {
    std::vector<AudioStream> streams;
    // Parse audio streams from JSON
    return streams;
}

} // namespace bilibili