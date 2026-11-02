#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <bilibili/Types.hpp>

class AudioPlayer {
public:
    enum class State {
        Stopped,
        Loading,
        Playing,
        Paused,
        Buffering,
        Error,
        Ended
    };

    using StateCallback = std::function<void(State)>;
    using ProgressCallback = std::function<void(float current, float total)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using MetadataCallback = std::function<void(const bilibili::AudioInfo&)>;

    AudioPlayer();
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) = delete;
    AudioPlayer& operator=(AudioPlayer&&) = delete;

    bool initialize();
    void shutdown();

    bool play(const bilibili::AudioStream& stream, const bilibili::AudioInfo& info = {});
    bool play_url(const std::string& url, const bilibili::AudioInfo& info = {});
    void pause();
    void resume();
    void stop();
    void seek(float position);
    void seek_time(int64_t milliseconds);
    void set_speed(float speed);
    void set_volume(float volume);
    void toggle_pause();

    void set_playlist(const std::vector<bilibili::PlaylistItem>& items, int start_index = 0);
    void play_next();
    void play_prev();
    void clear_playlist();

    void set_sleep_timer(int minutes);
    void cancel_sleep_timer();

    State get_state() const { return state_.load(); }
    float get_position() const { return position_; }
    int64_t get_current_time_ms() const { return current_time_ms_; }
    int64_t get_duration_ms() const { return duration_ms_; }
    float get_speed() const { return speed_; }
    float get_volume() const { return volume_; }
    bool has_playlist() const { return !playlist_.empty(); }
    int get_playlist_index() const { return playlist_index_; }
    const bilibili::AudioInfo* get_current_info() const { return current_info_; }

    void set_state_callback(StateCallback cb) { state_cb_ = std::move(cb); }
    void set_progress_callback(ProgressCallback cb) { progress_cb_ = std::move(cb); }
    void set_error_callback(ErrorCallback cb) { error_cb_ = std::move(cb); }
    void set_metadata_callback(MetadataCallback cb) { metadata_cb_ = std::move(cb); }

    using SleepCallback = std::function<void()>;
    void set_sleep_callback(SleepCallback cb) { sleep_cb_ = std::move(cb); }

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

    std::atomic<State> state_{State::Stopped};
    std::atomic<float> position_{0.0f};
    std::atomic<int64_t> current_time_ms_{0};
    std::atomic<int64_t> duration_ms_{0};
    std::atomic<float> speed_{1.0f};
    std::atomic<float> volume_{0.8f};

    std::vector<bilibili::PlaylistItem> playlist_;
    int playlist_index_ = -1;
    const bilibili::AudioInfo* current_info_ = nullptr;

    StateCallback state_cb_;
    std::function<void(float, float)> progress_cb_;
    ErrorCallback error_cb_;
    MetadataCallback metadata_cb_;
    std::function<void()> sleep_cb_;

    void emit_state(State new_state);
    void emit_progress(float current, float total);
    void emit_error(const std::string& msg);
    void emit_metadata(const bilibili::AudioInfo& info);
    void on_sleep_timer();
};