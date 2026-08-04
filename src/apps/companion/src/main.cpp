#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedSession.hpp>
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

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::app::WindowedSession;
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
    // A companion lives in the corner of a desktop, not in front of one.
    // Everything it draws is laid out on kSceneUnits whole units a side.
    // So the size is that many units rather than a dividing pixel count.
    constexpr std::uint32_t kPixelsPerUnit = 8;

    constexpr antwika::gfx::Size kWindowSize{
        .width = kPixelsPerUnit * antwika::companion::kSceneUnits,
        .height = kPixelsPerUnit * antwika::companion::kSceneUnits};

    // Where the companion waits between one session and the next.
    // Beside the working directory rather than beside the executable.
    // A companion is what somebody's session made, not a shipped asset.
    // So it does not belong in the directory a build writes.
    constexpr std::string_view kCompanionFile = "companion.json";

    void run(const RecordedRun &recorded)
    {
        // The metabolism the pet runs on, read off config.json once.
        const auto config =
            antwika::companion::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        // Movement is coalesced and idle movement is held back.
        // A press is the only input this application has.
        // Where the pointer went between two of them is unreadable here.
        const WindowedSessionDesc desc{
            .name = "Antwika Companion",
            .windowTitle = "Antwika Companion",
            .canvas = kWindowSize,
            .input =
                {.coalescePointerMotion = true, .thinIdleMotion = true},
            .replayPath = recorded.options.replayPath};

        WindowedSession session(logger, *backend, *inputBackend, desc);

        antwika::companion::announceHowToStop(
            logger, session.drawsNothing());

        // Fixed here rather than read from anywhere.
        // Nothing this application draws is hit-tested against text.
        // So a locale could not change what a recorded press means.
        // It is fixed anyway, and one rule is cheaper than two.
        // Changing the language is this line, as the window size is.
        const Translator translator{antwika::i18n::kDefaultLocale};

        const PetScene scene{translator};
        SystemSleeper sleeper;
        FilePetStore store{std::string(kCompanionFile)};

        // The debug console's picture, described in the tick path.
        // Against the size the window was asked for, as everything is.
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
                    antwika::console::consoleLoadPermitted(
                        recorded.options.recordPath.has_value(),
                        recorded.options.replayPath.has_value()),
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
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_companion", run);
}
