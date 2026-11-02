#include "network/HttpClient.hpp"
#include <curl/curl.h>
#include <mbedtls/ssl.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/error.h>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include "utils/Logger.hpp"

struct HttpClient::Impl {
    CURL* curl = nullptr;
    struct curl_slist* headers_ = nullptr;
    std::string user_agent_ = "SwitchAudiobook/0.1.0";
    std::string proxy_;
    std::string ca_bundle_;
    bool verify_ssl_ = true;
    int connect_timeout_ms_ = 10000;
    int total_timeout_ms_ = 30000;

    struct Cookie {
        std::string domain;
        std::string name;
        std::string value;
        std::string path;
        long expires = 0;
    };
    std::vector<Cookie> cookies_;
    std::mutex cookies_mutex_;

    struct Stats {
        std::atomic<size_t> total_requests{0};
        std::atomic<size_t> failed_requests{0};
        std::atomic<size_t> total_bytes_sent{0};
        std::atomic<size_t> total_bytes_received{0};
    } stats_;

    std::string current_response_body_;
    HttpResponse current_response_;

    Impl() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate, br");
            curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify_ssl_ ? 1L : 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify_ssl_ ? 2L : 0L);
        }
    }

    ~Impl() {
        if (curl) curl_easy_cleanup(curl);
        if (headers_) curl_slist_free_all(headers_);
        curl_global_cleanup();
    }

    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        Impl* self = static_cast<Impl*>(userp);
        size_t realsize = size * nmemb;
        std::string* body = &self->current_response_body_;
        body->append(static_cast<char*>(contents), realsize);
        self->stats_.total_bytes_received.fetch_add(realsize);
        return realsize;
    }

    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
        Impl* self = static_cast<Impl*>(userdata);
        size_t len = size * nitems;
        std::string header(buffer, len);

        if (header.rfind("Set-Cookie:", 0) == 0) {
            std::string cookie = header.substr(11);
            self->parse_cookie(cookie);
        }
        return len;
    }

    void parse_cookie(const std::string& cookie_str) {
        // Simplified cookie parsing
    }
};

HttpClient::HttpClient() : pimpl_(std::make_unique<Impl>()) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) = default;
HttpClient& HttpClient::operator=(HttpClient&&) = default;

HttpResponse HttpClient::get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    auto start = std::chrono::steady_clock::now();
    pimpl_->stats_.total_requests.fetch_add(1);

    curl_easy_setopt(pimpl_->curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(pimpl_->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pimpl_->curl, CURLOPT_POSTFIELDS, nullptr);

    curl_easy_setopt(pimpl_->curl, CURLOPT_CONNECTTIMEOUT_MS, pimpl_->connect_timeout_ms_);
    curl_easy_setopt(pimpl_->curl, CURLOPT_TIMEOUT_MS, pimpl_->total_timeout_ms_);

    if (pimpl_->headers_) {
        curl_slist_free_all(pimpl_->headers_);
        pimpl_->headers_ = nullptr;
    }
    for (const auto& h : headers) {
        pimpl_->headers_ = curl_slist_append(pimpl_->headers_, (h.first + ": " + h.second).c_str());
    }
    pimpl_->headers_ = curl_slist_append(pimpl_->headers_, ("User-Agent: " + pimpl_->user_agent_).c_str());

    std::string cookie_header = build_cookie_header();
    if (!cookie_header.empty()) {
        pimpl_->headers_ = curl_slist_append(pimpl_->headers_, ("Cookie: " + cookie_header).c_str());
    }
    curl_easy_setopt(pimpl_->curl, CURLOPT_HTTPHEADER, pimpl_->headers_);

    pimpl_->current_response_body_.clear();
    pimpl_->current_response_ = HttpResponse{};

    CURLcode res = curl_easy_perform(pimpl_->curl);
    auto end = std::chrono::steady_clock::now();
    pimpl_->current_response_.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    long http_code = 0;
    curl_easy_getinfo(pimpl_->curl, CURLINFO_RESPONSE_CODE, &http_code);
    pimpl_->current_response_.status_code = static_cast<int>(http_code);
    pimpl_->current_response_.body = std::move(pimpl_->current_response_body_);

    if (res != CURLE_OK) {
        pimpl_->current_response_.success = false;
        pimpl_->current_response_.error_message = curl_easy_strerror(res);
        pimpl_->stats_.failed_requests.fetch_add(1);
        Logger::error("HTTP GET failed: {} - {}", url, curl_easy_strerror(res));
    } else {
        pimpl_->current_response_.success = (http_code >= 200 && http_code < 300);
        if (!pimpl_->current_response_.success) {
            pimpl_->stats_.failed_requests.fetch_add(1);
        }
    }

    return pimpl_->current_response_;
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers) {
    auto start = std::chrono::steady_clock::now();
    pimpl_->stats_.total_requests.fetch_add(1);

    curl_easy_setopt(pimpl_->curl, CURLOPT_POST, 1L);
    curl_easy_setopt(pimpl_->curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(pimpl_->curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(pimpl_->curl, CURLOPT_POSTFIELDSIZE, body.size());

    curl_easy_setopt(pimpl_->curl, CURLOPT_CONNECTTIMEOUT_MS, pimpl_->connect_timeout_ms_);
    curl_easy_setopt(pimpl_->curl, CURLOPT_TIMEOUT_MS, pimpl_->total_timeout_ms_);

    if (pimpl_->headers_) {
        curl_slist_free_all(pimpl_->headers_);
        pimpl_->headers_ = nullptr;
    }
    for (const auto& h : headers) {
        pimpl_->headers_ = curl_slist_append(pimpl_->headers_, (h.first + ": " + h.second).c_str());
    }
    pimpl_->headers_ = curl_slist_append(pimpl_->headers_, ("User-Agent: " + pimpl_->user_agent_).c_str());

    std::string cookie_header = build_cookie_header();
    if (!cookie_header.empty()) {
        pimpl_->headers_ = curl_slist_append(pimpl_->headers_, ("Cookie: " + cookie_header).c_str());
    }
    curl_easy_setopt(pimpl_->curl, CURLOPT_HTTPHEADER, pimpl_->headers_);

    pimpl_->current_response_body_.clear();
    pimpl_->current_response_ = HttpResponse{};

    CURLcode res = curl_easy_perform(pimpl_->curl);
    auto end = std::chrono::steady_clock::now();
    pimpl_->current_response_.response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    long http_code = 0;
    curl_easy_getinfo(pimpl_->curl, CURLINFO_RESPONSE_CODE, &http_code);
    pimpl_->current_response_.status_code = static_cast<int>(http_code);
    pimpl_->current_response_.body = std::move(pimpl_->current_response_body_);

    if (res != CURLE_OK) {
        pimpl_->current_response_.success = false;
        pimpl_->current_response_.error_message = curl_easy_strerror(res);
        pimpl_->stats_.failed_requests.fetch_add(1);
    } else {
        pimpl_->current_response_.success = (http_code >= 200 && http_code < 300);
        if (!pimpl_->current_response_.success) {
            pimpl_->stats_.failed_requests.fetch_add(1);
        }
    }

    return pimpl_->current_response_;
}

void HttpClient::get_async(const std::string& url, Callback cb, const std::vector<std::pair<std::string, std::string>>& headers) {
    // Async implementation would use thread pool
    cb(get(url, headers));
}

void HttpClient::post_async(const std::string& url, const std::string& body, Callback cb, const std::vector<std::pair<std::string, std::string>>& headers) {
    cb(post(url, body, headers));
}

void HttpClient::set_timeout(int connect_timeout_ms, int total_timeout_ms) {
    pimpl_->connect_timeout_ms_ = connect_timeout_ms;
    pimpl_->total_timeout_ms_ = total_timeout_ms;
}

void HttpClient::set_user_agent(const std::string& ua) {
    pimpl_->user_agent_ = ua;
}

void HttpClient::set_proxy(const std::string& proxy_url) {
    pimpl_->proxy_ = proxy_url;
    curl_easy_setopt(pimpl_->curl, CURLOPT_PROXY, proxy_url.c_str());
}

void HttpClient::set_ca_bundle(const std::string& ca_path) {
    pimpl_->ca_bundle_ = ca_path;
    curl_easy_setopt(pimpl_->curl, CURLOPT_CAINFO, ca_path.c_str());
}

void HttpClient::set_verify_ssl(bool verify) {
    pimpl_->verify_ssl_ = verify;
    curl_easy_setopt(pimpl_->curl, CURLOPT_SSL_VERIFYPEER, verify ? 1L : 0L);
    curl_easy_setopt(pimpl_->curl, CURLOPT_SSL_VERIFYHOST, verify ? 2L : 0L);
}

void HttpClient::set_cookie(const std::string& domain, const std::string& name, const std::string& value) {
    std::lock_guard<std::mutex> lock(pimpl_->cookies_mutex_);
}

std::string HttpClient::get_cookie(const std::string& domain, const std::string& name) const {
    return "";
}

void HttpClient::clear_cookies() {
    std::lock_guard<std::mutex> lock(pimpl_->cookies_mutex_);
    pimpl_->cookies_.clear();
}

HttpClient::Stats HttpClient::get_stats() const {
    return pimpl_->stats_;
}

void HttpClient::reset_stats() {
    pimpl_->stats_ = typename Impl::Stats{};
}

std::string HttpClient::Impl::build_cookie_header() {
    std::string header;
    return header;
}