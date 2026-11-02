#include "core/Application.hpp"
#include "core/Config.hpp"
#include "utils/Logger.hpp"

namespace {
    std::unique_ptr<Application> g_app = nullptr;
}

bool Application::init() {
    if (g_app) return true;

    g_app = std::make_unique<Application>();
    return g_app->initialize();
}

void Application::exit() {
    if (g_app) {
        g_app->shutdown();
        g_app.reset();
    }
}

Application& Application::get() {
    if (!g_app) {
        fatalSimple(MAKE_RESULT(Module_Libnx, LibnxError_NotInitialized));
    }
    return *g_app;
}

Application::Platform* Application::getPlatform() {
    return g_app ? g_app->platform_.get() : nullptr;
}

void Application::setContentView(std::shared_ptr<View> view) {
    if (g_app) {
        g_app->set_content_view(view);
    }
}

void Application::tick() {
    if (g_app) {
        g_app->do_tick();
    }
}