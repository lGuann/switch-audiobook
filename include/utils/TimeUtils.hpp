#pragma once

#include <chrono>
#include <string>
#include <ctime>

namespace utils {

inline int64_t current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

inline int64_t current_time_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

inline std::string format_timestamp(int64_t timestamp_sec) {
    std::time_t t = static_cast<std::time_t>(timestamp_sec);
    std::tm tm = *std::localtime(&t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buffer);
}

inline std::string format_date(int64_t timestamp_sec) {
    std::time_t t = static_cast<std::time_t>(timestamp_sec);
    std::tm tm = *std::localtime(&t);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return std::string(buffer);
}

inline std::string format_time_ago(int64_t timestamp_sec) {
    int64_t now = std::time(nullptr);
    int64_t diff = now - timestamp_sec;

    if (diff < 60) return "刚刚";
    if (diff < 3600) return std::to_string(timestamp_sec / 60) + "分钟前";
    if (diff < 86400) return std::to_string(timestamp_sec / 3600) + "小时前";
    if (diff < 604800) return std::to_string(timestamp_sec / 86400) + "天前";
    if (diff < 2592000) return std::to_string(timestamp_sec / 604800) + "周前";
    if (diff < 31536000) return std::to_string(timestamp_sec / 2592000) + "个月前";
    return std::to_string(timestamp_sec / 31536000) + "年前";
}

inline int64_t parse_iso8601(const std::string& iso_str) {
    std::tm tm = {};
    std::istringstream ss(iso_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return 0;
    return std::mktime(&tm);
}

} // namespace utils