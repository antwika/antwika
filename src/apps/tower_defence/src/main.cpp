#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/RenderSink.hpp"
#include "antwika/tower_defence/TowerDefence.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::replay::WindowInputSource;
using antwika::time::SystemSleeper;
using antwika::tower_defence::Battle;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::BattleSummary;
using antwika::tower_defence::RenderSink;
using antwika::tower_defence::ScoreOverlay;

namespace
{
    // A whole number of pixels per cell at the default level size.
    // The score bar's strip comes off the top first.
    constexpr antwika::gfx::Size kWindowSize{
        .width = 960, .height = 720};

    constexpr std::chrono::milliseconds kFramePeriod{80};

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika Tower Defence on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Tower Defence",
            .size = kWindowSize,
            .resizable = false});

        const BattleScene scene;
        SystemSleeper sleeper;

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // A replay already holds the input it recorded.
        // Movement is coalesced and idle movement held back.
        // Nothing here follows a free-moving pointer.
        // Only presses build, so what is between them is not recorded.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true,
             .thinIdleMotion = true});

        WindowInputSource source(input, *backend, window->id());

        const BattleSummary summary =
            antwika::tower_defence::bootstrap({
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .codec = codec,
                .canvas = kWindowSize,
                .replayRecorder = recorded.replayRecorder,
                .extraSink =
                    [&](const Battle &battle, const ScoreOverlay &overlay)
                {
                    return std::make_unique<RenderSink>(
                        *window,
                        scene,
                        battle,
                        overlay,
                        sleeper,
                        kFramePeriod,
                        kWindowSize);
                }});

        logger.log(
            Level::Info,
            "Final score " + std::to_string(summary.score) + " over "
                + std::to_string(summary.ticks) + " ticks, "
                + std::to_string(summary.leaks) + " leaked");
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_tower_defence", run);
}
