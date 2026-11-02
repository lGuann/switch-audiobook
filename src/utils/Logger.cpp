#include "utils/Logger.hpp"

Logger::Level Logger::current_level_ = Logger::Level::Info;
std::ofstream Logger::log_file_;
std::mutex Logger::log_mutex_;
std::string Logger::log_path_;

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
        << fmt << "\n";

    std::string message = oss.str();

    if (log_file_.is_open()) {
        log_file_ << message;
        log_file_.flush();
    }

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