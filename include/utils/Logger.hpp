#pragma once

#include <string>
#include <chrono>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>

class Logger {
public:
    enum class Level {
        Debug,
        Info,
        Warning,
        Error
    };

    static void init(const std::string& log_path = "/switch/switch-audiobook/logs/app.log");
    static void set_level(Level level);

    template<typename... Args>
    static void debug(const std::string& fmt, Args&&... args) {
        log(Level::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void info(const std::string& fmt, Args&&... args) {
        log(Level::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void warning(const std::string& fmt, Args&&... args) {
        log(Level::Warning, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void error(const std::string& fmt, Args&&... args) {
        log(Level::Error, fmt, std::forward<Args>(args)...);
    }

private:
    static Level current_level_;
    static std::ofstream log_file_;
    static std::mutex log_mutex_;
    static std::string log_path_;

    template<typename... Args>
    static void log(Level level, const std::string& fmt, Args&&... args);

    static std::string format_time();
    static std::string level_to_string(Level level);
    static std::string format_message(const std::string& fmt);
};

inline Logger::Level Logger::current_level_ = Logger::Level::Info;
inline std::ofstream Logger::log_file_;
inline std::mutex Logger::log_mutex_;
inline std::string Logger::log_path_;

void Logger::init(const std::string& log_path) {
    log_path_ = log_path;
    log_file_.open(log_path_, std::ios::app);
}

void Logger::set_level(Level level) {
    current_level_ = level;
}

template<typename... Args>
void Logger::log(Level level, const std::string& fmt, Args&&... args) {
    if (level < current_level_) return;

    std::lock_guard<std::mutex> lock(log_mutex_);

    std::ostringstream oss;
    oss << format_time() << " [" << level_to_string(level) << "] "
        << format_message(fmt) << "\n";

    std::string message = oss.str();

    if (log_file_.is_open()) {
        log_file_ << message;
        log_file_.flush();
    }

    // Also output to stdout for debugging
    std::cout << message;
}

std::string Logger::format_time() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()) % 1000;

    std::tm tm = *std::localtime(&time_t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string Logger::level_to_string(Level level) {
    switch (level) {
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::format_message(const std::string& fmt) {
    return fmt; // Simplified - in real impl would format with args
}