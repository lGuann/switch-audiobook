#include "player/AudioPlayer.hpp"
#include <mpv/client.h>
#include <switch.h>
#include <thread>
#include <chrono>
#include <mutex>
#include "utils/Logger.hpp"

struct AudioPlayer::Impl {
    mpv_handle* mpv = nullptr;
    std::thread event_thread;
    std::atomic<bool> running{false};
    std::mutex mutex;

    std::vector<bilibili::PlaylistItem> playlist;
    int playlist_index = -1;
    const bilibili::AudioInfo* current_info = nullptr;

    std::function<void(State)> state_cb;
    std::function<void(float, float)> progress_cb;
    std::function<void(const std::string&)> error_cb;
    std::function<void(const bilibili::AudioInfo&)> metadata_cb;
    std::function<void()> sleep_cb;

    int sleep_timer_minutes = 0;
    std::thread sleep_thread;
    std::atomic<bool> sleep_active{false};

    Impl() = default;
    ~Impl() {
        shutdown();
    }

    void shutdown() {
        running = false;
        sleep_active = false;
        if (sleep_thread.joinable()) sleep_thread.join();
        if (event_thread.joinable()) event_thread.join();
        if (mpv) {
            mpv_terminate_destroy(mpv);
            mpv = nullptr;
        }
    }

    bool initialize() {
        mpv = mpv_create();
        if (!mpv) return false;

        mpv_set_option_string(mpv, "vo", "null");
        mpv_set_option_string(mpv, "ao", "sdl2");
        mpv_set_option_string(mpv, "audio-buffer", "2.0");
        mpv_set_option_string(mpv, "audio-channels", "stereo");
        mpv_set_option(mpv, "volume", MPV_FORMAT_DOUBLE, &(double){0.8});
        mpv_set_option(mpv, "speed", MPV_FORMAT_DOUBLE, &(double){1.0});

        int ret = mpv_initialize(mpv);
        if (ret < 0) return false;

        running = true;
        event_thread = std::thread(&Impl::event_loop, this);
        return true;
    }

    void event_loop() {
        while (running) {
            mpv_event* event = mpv_wait_event(mpv, 0.1);
            if (!event) continue;

            switch (event->event_id) {
                case MPV_EVENT_FILE_LOADED:
                    emit_state(State::Playing);
                    if (metadata_cb && current_info) metadata_cb(*current_info);
                    break;
                case MPV_EVENT_PLAYBACK_RESTART:
                    emit_state(State::Playing);
                    break;
                case MPV_EVENT_PAUSE:
                    emit_state(State::Paused);
                    break;
                case MPV_EVENT_UNPAUSE:
                    emit_state(State::Playing);
                    break;
                case MPV_EVENT_END_FILE: {
                    mpv_event_end_file* ef = (mpv_event_end_file*)event->data;
                    if (ef->reason == MPV_END_FILE_REASON_EOF) {
                        emit_state(State::Ended);
                        if (auto_play_next_) play_next();
                    } else if (ef->reason == MPV_END_FILE_REASON_ERROR) {
                        emit_state(State::Error);
                        if (error_cb) error_cb("Playback error");
                    } else {
                        emit_state(State::Stopped);
                    }
                    break;
                }
                case MPV_EVENT_TIME_POSITION:
                    update_progress();
                    break;
                case MPV_EVENT_PROPERTY_CHANGE: {
                    mpv_event_property* prop = (mpv_event_property*)event->data;
                    if (strcmp(prop->name, "time-pos") == 0) {
                        update_progress();
                    } else if (strcmp(prop->name, "duration") == 0) {
                        double* dur = (double*)prop->data;
                        if (dur) {
                            duration_ms_ = static_cast<int64_t>(*dur * 1000);
                        }
                    }
                    break;
                }
            }
        }
    }

    void update_progress() {
        double pos = 0, dur = 0;
        mpv_get_property(mpv, "time-pos", MPV_FORMAT_DOUBLE, &pos);
        mpv_get_property(mpv, "duration", MPV_FORMAT_DOUBLE, &dur);

        current_time_ms_ = static_cast<int64_t>(pos * 1000);
        duration_ms_ = static_cast<int64_t>(dur * 1000);

        float progress = (dur > 0) ? (float)(pos / dur) : 0.0f;
        position_ = progress;
        if (progress_cb) progress_cb(pos, dur);
    }

    void emit_state(State new_state) {
        if (state_cb) state_cb(new_state);
    }

    void emit_error(const std::string& msg) {
        if (error_cb) error_cb(msg);
    }

    void emit_metadata(const bilibili::AudioInfo& info) {
        if (metadata_cb) metadata_cb(info);
    }

    void on_sleep_timer() {
        if (sleep_cb) sleep_cb();
        stop();
    }

    void start_sleep_timer(int minutes) {
        sleep_active = true;
        sleep_timer_minutes = minutes;
        sleep_thread = std::thread([this, minutes]() {
            std::this_thread::sleep_for(std::chrono::minutes(minutes));
            if (sleep_active) on_sleep_timer();
        });
    }

    void cancel_sleep_timer() {
        sleep_active = false;
        if (sleep_thread.joinable()) sleep_thread.join();
    }
};

AudioPlayer::AudioPlayer() : pimpl_(std::make_unique<Impl>()) {}

AudioPlayer::~AudioPlayer() = default;

bool AudioPlayer::initialize() {
    return pimpl_->initialize();
}

void AudioPlayer::shutdown() {
    pimpl_->shutdown();
}

bool AudioPlayer::play(const bilibili::AudioStream& stream, const bilibili::AudioInfo& info) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex);
    pimpl_->current_info = &info;
    std::string cmd = "loadfile " + stream.url;
    int ret = mpv_command_string(pimpl_->mpv, cmd.c_str());
    return ret >= 0;
}

bool AudioPlayer::play_url(const std::string& url, const bilibili::AudioInfo& info) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex);
    pimpl_->current_info = &info;
    std::string cmd = "loadfile " + url;
    int ret = mpv_command_string(pimpl_->mpv, cmd.c_str());
    return ret >= 0;
}

void AudioPlayer::pause() {
    mpv_set_property_string(pimpl_->mpv, "pause", "yes");
}

void AudioPlayer::resume() {
    mpv_set_property_string(pimpl_->mpv, "pause", "no");
}

void AudioPlayer::stop() {
    mpv_command_string(pimpl_->mpv, "stop");
}

void AudioPlayer::seek(float position) {
    double dur = 0;
    mpv_get_property(pimpl_->mpv, "duration", MPV_FORMAT_DOUBLE, &dur);
    if (dur > 0) {
        mpv_set_property(pimpl_->mpv, "time-pos", MPV_FORMAT_DOUBLE, &(double){dur * position});
    }
}

void AudioPlayer::seek_time(int64_t milliseconds) {
    mpv_set_property(pimpl_->mpv, "time-pos", MPV_FORMAT_DOUBLE, &(double){milliseconds / 1000.0});
}

void AudioPlayer::set_speed(float speed) {
    mpv_set_property(pimpl_->mpv, "speed", MPV_FORMAT_DOUBLE, &(double){speed});
}

void AudioPlayer::set_volume(float volume) {
    mpv_set_property(pimpl_->mpv, "volume", MPV_FORMAT_DOUBLE, &(double){volume});
}

void AudioPlayer::toggle_pause() {
    int paused = 0;
    mpv_get_property(pimpl_->mpv, "pause", MPV_FORMAT_FLAG, &paused);
    mpv_set_property_string(pimpl_->mpv, "pause", paused ? "no" : "yes");
}

void AudioPlayer::set_playlist(const std::vector<bilibili::PlaylistItem>& items, int start_index) {
    pimpl_->playlist = items;
    pimpl_->playlist_index = start_index;
    if (start_index >= 0 && start_index < (int)items.size()) {
        // Would need to convert playlist item to stream URL
    }
}

void AudioPlayer::play_next() {
    if (pimpl_->playlist_index + 1 < (int)pimpl_->playlist.size()) {
        pimpl_->playlist_index++;
        // play playlist[pimpl_->playlist_index]
    }
}

void AudioPlayer::play_prev() {
    if (pimpl_->playlist_index > 0) {
        pimpl_->playlist_index--;
        // play playlist[pimpl_->playlist_index]
    }
}

void AudioPlayer::clear_playlist() {
    pimpl_->playlist.clear();
    pimpl_->playlist_index = -1;
}

void AudioPlayer::set_sleep_timer(int minutes) {
    cancel_sleep_timer();
    if (minutes > 0) {
        pimpl_->start_sleep_timer(minutes);
    }
}

void AudioPlayer::cancel_sleep_timer() {
    pimpl_->cancel_sleep_timer();
}

void AudioPlayer::set_state_callback(StateCallback cb) {
    pimpl_->state_cb = std::move(cb);
}

void AudioPlayer::set_progress_callback(ProgressCallback cb) {
    pimpl_->progress_cb = std::move(cb);
}

void AudioPlayer::set_error_callback(ErrorCallback cb) {
    pimpl_->error_cb = std::move(cb);
}

void AudioPlayer::set_metadata_callback(MetadataCallback cb) {
    pimpl_->metadata_cb = std::move(cb);
}

void AudioPlayer::set_sleep_callback(SleepCallback cb) {
    pimpl_->sleep_cb = std::move(cb);
}