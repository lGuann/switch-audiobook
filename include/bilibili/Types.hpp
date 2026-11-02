#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace bilibili {

struct AudioInfo {
    std::string aid;
    std::string bvid;
    std::string cid;
    std::string title;
    std::string author;
    std::string cover;
    int duration = 0;
    int pubdate = 0;
    std::string description;
    long long view_count = 0;
    long long like_count = 0;
    long long favorite_count = 0;
    std::vector<std::string> tags;
};

struct AudioStream {
    std::string url;
    std::string quality;
    std::string format;
    long long size = 0;
    int duration = 0;
    std::string codec;
    int bitrate = 0;
};

struct SearchResult {
    std::string bvid;
    std::string title;
    std::string author;
    std::string cover;
    int duration = 0;
    int pubdate = 0;
    std::string description;
    long long view_count = 0;
    bool is_audio = false;
    int audio_id = 0;
};

struct SeasonInfo {
    std::string season_id;
    std::string title;
    std::string cover;
    int total_count = 0;
    std::vector<SearchResult> episodes;
};

struct UserInfo {
    int mid = 0;
    std::string name;
    std::string face;
    std::string sign;
    int level = 0;
    long long follower = 0;
};

struct PlaylistItem {
    std::string bvid;
    std::string cid;
    std::string title;
    int duration = 0;
    std::string cover;
    int page = 0;
};

void from_json(const nlohmann::json& j, AudioInfo& info);
void from_json(const nlohmann::json& j, AudioStream& stream);
void from_json(const nlohmann::json& j, SearchResult& result);
void from_json(const nlohmann::json& j, SeasonInfo& season);
void from_json(const nlohmann::json& j, UserInfo& user);
void from_json(const nlohmann::json& j, PlaylistItem& item);

struct ApiResponse {
    int code = -1;
    std::string message;
    int ttl = 1;
    nlohmann::json data;
    bool success() const { return code == 0; }
};

template<typename T>
struct TypedApiResponse : ApiResponse {
    T data;
};

} // namespace bilibili