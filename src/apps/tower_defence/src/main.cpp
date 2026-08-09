#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedHost.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include <antwika/app/AssetPath.hpp>
#include "antwika/tower_defence/ConfigFile.hpp"
#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/FileScoreStore.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/RenderSink.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
using antwika::app::WindowedSessionDesc;
using antwika::log::Level;
using antwika::time::SystemSleeper;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::FileScoreStore;
using antwika::tower_defence::RenderSink;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::Translator;

namespace
{
    constexpr antwika::gfx::Size kWindowSize{
        .width = 960, .height = 720};

    constexpr std::string_view kScoreFile =
        "tower_defence_highscore.json";

    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::tower_defence::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika Tower Defence",
                .windowTitle = "Antwika Tower Defence",
                .canvas = kWindowSize,
                .input =
                    {.coalescePointerMotion = true, .thinIdleMotion = true},
                .replayPath = recorded.options.replayPath});

        auto &logger = host.logger();
        auto &session = host.session();

        const Translator translator{antwika::i18n::kDefaultLocale};

        const BattleScene scene;
        SystemSleeper sleeper;
        FileScoreStore store{std::string(kScoreFile)};

        antwika::console::ConsolePicture consoleOverlay(session.canvas());

        const BattleSummary summary =
            antwika::tower_defence::bootstrap({
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = session.source(),
                .codec = session.codec(),
                .translator = translator,
                .canvas = kWindowSize,
                .campaign =
                    {.lives = config.startingLives,
                     .mobs = config.mobs},
                .scoreStore = antwika::tower_defence::storeIfLive(
                    store, recorded.options.replayPath),
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled =
                    antwika::console::consoleLoadPermitted(recorded.options),
                .replayRecorder = recorded.replayRecorder,
                .extraSink =
                    [&](const Campaign &campaign,
                        const ScoreOverlay &overlay)
                {
                    return std::make_unique<RenderSink>(
                        session.window(),
                        scene,
                        campaign,
                        overlay,
                        consoleOverlay,
                        sleeper,
                        std::chrono::milliseconds(
                            config.framePeriodMs),
                        kWindowSize);
                }});

        logger.log(
            Level::Info, antwika::tower_defence::summaryLine(summary));
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_tower_defence", run);
}
