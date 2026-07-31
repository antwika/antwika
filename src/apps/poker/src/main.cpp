#include "antwika/poker/PokerRoom.hpp"

#include <iostream>
#include <string>
#include <string_view>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/PngFile.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

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
    constexpr std::string_view kDemoReplayPath = ANTWIKA_POKER_DEMO_REPLAY_PATH;
    const std::string kAtlasPath = ANTWIKA_POKER_ATLAS_PATH;

    void run(const RecordedRun &recorded)
    {
        const auto watch =
            antwika::poker::watchOptionsFrom(recorded.commandLine);

        ConsoleLogging logging(std::cout, Level::Warning);
        SystemClock clock;
        SystemSleeper sleeper;
        ReplaySource source(antwika::app::scriptedEvents(
            recorded.options.replayPath, kDemoReplayPath));

        // The window is always opened, as in the gfx demo.
        // Under the headless backend it draws and costs nothing.
        const auto backend =
            antwika::gfx::makeSelectedBackend(logging.logger());

        // antwika::gfx opens no files, so the app reads the atlas.
        const auto atlas =
            antwika::app::readPngFile(kAtlasPath, "antwika_poker");
        const WindowSetup window{
            .backend = *backend,
            .sleeper = sleeper,
            .framePeriod = watch.tickDelay,
            .holdFinalFrame = watch.holdFinalFrame,
            .atlas = &atlas,
        };

        antwika::poker::printSummary(
            std::cout,
            antwika::poker::bootstrap(
                antwika::poker::RoomSetup{
                    .clock = clock,
                    .logger = logging.logger(),
                    .eventSink = recorded.eventSink,
                    .inputSource = source,
                    .out = std::cout,
                    .replayRecorder = recorded.replayRecorder,
                    .window = window}));
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_poker", run, antwika::poker::watchFlags());
}
