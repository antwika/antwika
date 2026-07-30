#include "antwika/game/Game.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/CoalescingPointerSource.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/LiveInputSource.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Events.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/TickPacer.hpp"
#include "antwika/game/WindowInputSource.hpp"

using antwika::ecs::ISystem;
using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::game::Camera;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::PathIndex;
using antwika::game::RenderSystem;
using antwika::game::TickPacer;
using antwika::game::WindowInputSource;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::input::CoalescingPointerSource;
using antwika::input::InputEventCodec;
using antwika::input::LiveInputSource;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;
using antwika::time::SystemSleeper;

namespace
{
    constexpr Size kWindowSize{.width = 1024, .height = 640};
    constexpr GridExtent kExtent{.width = 24, .height = 24};

    // The origin cell's top corner starts here.
    // The projection is anchored to the camera, not the canvas centre.
    // So this constant is what puts the grid in view.
    // And it is why a resize cannot change which cell a pixel means.
    constexpr Point kInitialPan{.x = 512, .y = 48};

    constexpr std::chrono::milliseconds kTickInterval{40};

    // The backend that draws nothing.
    // A build using it has nothing to watch and nothing to wait for.
    constexpr std::string_view kHeadlessBackendName = "null";

    constexpr std::string_view kDemoReplayPath = ANTWIKA_GAME_DEMO_REPLAY_PATH;

    // Only this app's own announcement is filtered from a recording.
    // No input.* name may ever join it.
    // That would stop recording the only input a live run has.
    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        antwika::game::events::kStarted,
    };

    void printSummary(const antwika::game::GameSummary &summary)
    {
        std::cout << "Final state: ticksProcessed="
                  << summary.state.ticksProcessed
                  << " score=" << summary.state.score << '\n';
        std::cout << "Paths laid: " << summary.paths.size() << '\n';
        std::cout << "Walkers: " << summary.walkers.size() << '\n';

        for (const auto &walker : summary.walkers)
        {
            std::cout << "  at (" << walker.at.x << ", " << walker.at.y
                      << ") facing "
                      << antwika::game::directionIndex(walker.facing)
                      << '\n';
        }

        std::cout << "Camera: pan (" << summary.camera.pan().x << ", "
                  << summary.camera.pan().y << ") zoom "
                  << summary.camera.zoomLevel() << '\n';
    }
} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);
    EventRecorder eventSink;
    TickEventRecorder replayRecorder;

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // Catching here also lets a failed --record run save what it has.
    int exitCode = EXIT_SUCCESS;
    try
    {
        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);
        const bool showsNothing =
            backend->name() == kHeadlessBackendName;

        logger.log(
            Level::Info,
            "Antwika Game on backends: " + std::string(backend->name())
                + " / " + std::string(inputBackend->name()));

        const auto window = backend->createWindow(
            WindowDesc{.title = "Antwika Game", .size = kWindowSize});

        Camera camera(kInitialPan);
        PathIndex paths;
        const GridScene scene;
        RenderSystem renderSystem(
            *window, scene, paths, camera, kExtent);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        // A backend showing nothing gives nobody a reason to wait.
        std::vector<std::reference_wrapper<ISystem>> observers{
            renderSystem};
        if (!showsNothing)
        {
            observers.emplace_back(pacer);
        }

        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource fileSource(std::move(events));

        // A --replay run must not attach a live source.
        // Every input would arrive twice: from the file and the device.
        const bool live = !options.replayPath.has_value();
        const InputEventCodec codec;
        CoalescingPointerSource coalesced(fileSource);
        LiveInputSource liveSource(coalesced, *inputBackend, codec);
        antwika::replay::IReplaySource &inner =
            live ? static_cast<antwika::replay::IReplaySource &>(liveSource)
                 : static_cast<antwika::replay::IReplaySource &>(fileSource);
        WindowInputSource source(inner, *backend, window->id());

        const auto summary = antwika::game::bootstrap(
            logger,
            eventSink,
            source,
            codec,
            kExtent,
            camera,
            paths,
            observers,
            std::nullopt,
            &replayRecorder);

        printSummary(summary);
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_game: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(),
            *options.recordPath,
            kSelfGeneratedEventNames);
    }

    return exitCode;
}
