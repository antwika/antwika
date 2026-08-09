#include <iostream>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/poker/ConfigFile.hpp"
#include "antwika/poker/PokerRoom.hpp"
#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/WatchOptions.hpp"
#include "antwika/poker/WindowSetup.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::log::Level;
using antwika::poker::WindowSetup;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;
using antwika::time::SystemSleeper;

namespace
{
    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::poker::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        const auto watch =
            antwika::poker::watchOptionsFrom(recorded.commandLine);

        ConsoleLogging logging(std::cout, Level::Warning);
        SystemClock clock;
        SystemSleeper sleeper;
        ReplaySource source(antwika::app::scriptedEvents(
            recorded.options.replayPath,
            antwika::app::assetPath("demo.jsonl")));

        const auto backend =
            antwika::gfx::makeSelectedBackend(logging.logger());

        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logging.logger());
        const antwika::input::InputEventCodec codec;
        antwika::input::InputPipeline input(
            source,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .thinIdleMotion = true});

        const auto atlas = antwika::app::readPngFile(
            antwika::app::assetPath("atlas.png"), "antwika_poker");
        const WindowSetup window{
            .backend = *backend,
            .sleeper = sleeper,
            .framePeriod = watch.tickDelay,
            .holdFinalFrame = watch.holdFinalFrame,
            .atlas = &atlas,
        };

        antwika::console::ConsolePicture consolePicture(window.size);

        antwika::poker::printSummary(
            std::cout,
            antwika::poker::bootstrap(
                antwika::poker::RoomSetup{
                    .clock = clock,
                    .logger = logging.logger(),
                    .eventSink = recorded.eventSink,
                    .inputSource = input,
                    .out = std::cout,
                    .room = config,
                    .codec = codec,
                    .consoleOverlay = consolePicture,
                    .consoleLoadEnabled =
                        antwika::console::consoleLoadPermitted(
                            recorded.options),
                    .replayRecorder = recorded.replayRecorder,
                    .window = window}));
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_poker", run, antwika::poker::watchFlags());
}
