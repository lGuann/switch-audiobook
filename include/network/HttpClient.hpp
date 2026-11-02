#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

struct HttpResponse {
    int status_code = 0;
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;
    bool success = false;
    std::string error_message;
    long response_time_ms = 0;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = default;
    HttpClient& operator=(HttpClient&&) = default;

    HttpResponse get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers = {});
    HttpResponse post(const std::string& url, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers = {});

    using Callback = std::function<void(const HttpResponse&)>;
    void get_async(const std::string& url, Callback cb, const std::vector<std::pair<std::string, std::string>>& headers = {});
    void post_async(const std::string& url, const std::string& body, Callback cb, const std::vector<std::pair<std::string, std::string>>& headers = {});

    void set_timeout(int connect_timeout_ms, int total_timeout_ms);
    void set_user_agent(const std::string& ua);
    void set_proxy(const std::string& proxy_url);
    void set_ca_bundle(const std::string& ca_path);
    void set_verify_ssl(bool verify);

    void set_cookie(const std::string& domain, const std::string& name, const std::string& value);
    std::string get_cookie(const std::string& domain, const std::string& name) const;
    void clear_cookies();

    struct Stats {
        size_t total_requests = 0;
        size_t failed_requests = 0;
        size_t total_bytes_sent = 0;
        size_t total_bytes_received = 0;
    };
    Stats get_stats() const;
    void reset_stats();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;
};