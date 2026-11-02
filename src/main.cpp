#include <switch.h>
#include <borealis.hpp>
#include <memory>
#include <bilibili/BiliBiliClient.hpp>
#include <network/HttpClient.hpp>
#include "core/Application.hpp"
#include "core/Config.hpp"
#include "player/AudioPlayer.hpp"
#include "ui/MainUI.hpp"
#include "utils/Logger.hpp"

using namespace brls;

int main(int argc, char* argv[]) {
    Result rc = romfsInit();
    if (R_FAILED(rc)) {
        fatalSimple(rc);
    }

    socketInitializeDefault();

    if (!Application::init()) {
        Logger::error("Failed to initialize borealis");
        return -1;
    }

    Application::getPlatform()->setTheme(ThemeType::Dark);

    auto config = std::make_shared<Config>();
    config->load();

    auto http_client = std::make_shared<HttpClient>();
    http_client->set_timeout(10000, 30000);
    http_client->set_user_agent("SwitchAudiobook/0.1.0 (Nintendo Switch; libnx)");

    http_client->set_ca_bundle("romfs:/ca-bundle.crt");
    http_client->set_verify_ssl(true);

    auto bili_client = std::make_shared<bilibili::BiliBiliClient>(http_client, config);

    if (config->has_valid_cookies()) {
        bili_client->login(
            config->get_sessdata(),
            config->get_bili_jct(),
            config->get_buvid3()
        );
    }

    auto audio_player = std::make_shared<AudioPlayer>();
    if (!audio_player->initialize()) {
        Logger::error("Failed to initialize audio player");
    }

    auto main_ui = std::make_shared<MainUI>(bili_client, audio_player, config);

    Application::setContentView(main_ui);

    while (appletMainLoop()) {
        Application::tick();
    }

    Application::exit();
    audio_player->shutdown();
    config->save();
    http_client->clear_cookies();

    socketExit();
    romfsExit();

    return 0;
}