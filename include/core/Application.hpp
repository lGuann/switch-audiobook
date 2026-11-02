#pragma once

#include <memory>
#include <borealis.hpp>

class Application {
public:
    struct Platform {
        virtual ~Platform() = default;
        virtual void setTheme(int theme) = 0;
    };

    static bool init();
    static void exit();
    static Application& get();
    static Platform* getPlatform();
    static void setContentView(std::shared_ptr<brls::View> view);
    static void tick();

private:
    Application() = default;
    ~Application() = default;

    bool initialize();
    void shutdown();
    void set_content_view(std::shared_ptr<brls::View> view);
    void do_tick();

    std::unique_ptr<Platform> platform_;
    std::shared_ptr<brls::View> content_view_;

    static std::unique_ptr<Application> instance_;
};