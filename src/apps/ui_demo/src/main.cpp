#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/InputPipeline.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/replay/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/RenderSink.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/TickBudgetSource.hpp"
#include "antwika/ui_demo/UiDemo.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::input::InputEventCodec;
using antwika::input::InputPipeline;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::replay::WindowInputSource;
using antwika::time::SystemSleeper;
using antwika::ui_demo::DemoOverlay;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::DemoSummary;
using antwika::ui_demo::RenderSink;
using antwika::ui_demo::showcaseName;
using antwika::ui_demo::TickBudgetSource;

namespace
{
    constexpr antwika::gfx::Size kWindowSize{
        .width = 960, .height = 720};

    constexpr std::chrono::milliseconds kFramePeriod{40};

    // Capped, rather than run until the window is closed.
    // The default null backend reports no close at all.
    // It is also the build every CI leg produces.
    // An uncapped run there would never finish.
    // At the frame period above this is about a minute of showcase.
    constexpr antwika::time::Tick kTickBudget = 1500;

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika ui demo on backend: "
                + std::string(backend->name()) + ", input: "
                + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika ui demo",
            .size = kWindowSize,
            .resizable = false});

        const DemoScene scene;
        SystemSleeper sleeper;

        ReplaySource fileSource(
            antwika::app::scriptedEvents(recorded.options.replayPath));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // Movement is coalesced, since a layout reads one position.
        // Idle movement is deliberately not held back here.
        // A showcase wants a button lighting up as the pointer nears.
        // That appearance is antwika::ui resolving the event stream.
        InputPipeline input(
            fileSource,
            *inputBackend,
            codec,
            {.readsDevice = !recorded.options.replayPath.has_value(),
             .coalescePointerMotion = true});

        WindowInputSource windowed(input, *backend, window->id());
        TickBudgetSource source(windowed, kTickBudget);

        const DemoSummary summary = antwika::ui_demo::bootstrap({
            .logger = logger,
            .eventSink = recorded.eventSink,
            .inputSource = source,
            .codec = codec,
            .canvas = kWindowSize,
            .replayRecorder = recorded.replayRecorder,
            .extraSink =
                [&](const DemoState &, const DemoOverlay &overlay)
            {
                return std::make_unique<RenderSink>(
                    *window, scene, overlay, sleeper, kFramePeriod);
            }});

        logger.log(
            Level::Info,
            "Finished on the "
                + std::string{showcaseName(summary.showcase)}
                + " page, having counted "
                + std::to_string(summary.clicks) + " clicks");
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(argc, argv, "antwika_ui_demo", run);
}
