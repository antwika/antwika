#include <cstdint>
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
#include "antwika/companion/ConfigFile.hpp"
#include "antwika/companion/Companion.hpp"
#include "antwika/companion/FilePetStore.hpp"
#include "antwika/companion/Messages.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/RenderSink.hpp"

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
using antwika::app::WindowedSessionDesc;
using antwika::companion::CompanionSummary;
using antwika::companion::FilePetStore;
using antwika::companion::Lineage;
using antwika::companion::Pet;
using antwika::companion::PetScene;
using antwika::companion::RenderSink;
using antwika::companion::Translator;
using antwika::log::Level;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::uint32_t kPixelsPerUnit = 8;

    constexpr antwika::gfx::Size kWindowSize{
        .width = kPixelsPerUnit * antwika::companion::kSceneUnits,
        .height = kPixelsPerUnit * antwika::companion::kSceneUnits};

    constexpr std::string_view kCompanionFile = "companion.json";

    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::companion::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika Companion",
                .windowTitle = "Antwika Companion",
                .canvas = kWindowSize,
                .input =
                    {.coalescePointerMotion = true, .thinIdleMotion = true},
                .replayPath = recorded.options.replayPath});

        auto &logger = host.logger();
        auto &session = host.session();

        antwika::companion::announceHowToStop(
            logger, session.drawsNothing());

        const Translator translator{antwika::i18n::kDefaultLocale};

        const PetScene scene{translator};
        SystemSleeper sleeper;
        FilePetStore store{std::string(kCompanionFile)};

        antwika::console::ConsolePicture consoleOverlay(session.canvas());

        const CompanionSummary summary =
            antwika::companion::bootstrap({
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = session.source(),
                .codec = session.codec(),
                .sleeper = sleeper,
                .pet = config,
                .canvas = kWindowSize,
                .store = antwika::companion::storeIfLive(
                    store, recorded.options.replayPath),
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled =
                    antwika::console::consoleLoadPermitted(recorded.options),
                .replayRecorder = recorded.replayRecorder,
                .extraSink =
                    [&](const Pet &pet, const Lineage &lineage)
                {
                    return std::make_unique<RenderSink>(
                        session.window(),
                        scene,
                        pet,
                        lineage,
                        kWindowSize,
                        consoleOverlay);
                }});

        logger.log(
            Level::Info, antwika::companion::summaryLine(summary));
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_companion", run);
}
