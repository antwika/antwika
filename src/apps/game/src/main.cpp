#include "antwika/game/Game.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/event/EventRecorder.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PngReader.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/CoalescingPointerSource.hpp>
#include <antwika/input/IdleMotionSource.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/LiveInputSource.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/input/StopOnKeySource.hpp>
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
#include "antwika/game/UiOverlay.hpp"
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
using antwika::game::UiOverlay;
using antwika::game::WindowInputSource;
using antwika::gfx::Point;
using antwika::gfx::PngReader;
using antwika::gfx::Size;
using antwika::gfx::WindowDesc;
using antwika::input::CoalescingPointerSource;
using antwika::input::IdleMotionSource;
using antwika::input::InputEventCodec;
using antwika::input::LiveInputSource;
using antwika::input::StopOnKeySource;
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

    // Escape ends a live run, and so does closing the window.
    // Neither is available under the headless backend.
    // That build therefore runs until it is interrupted.
    constexpr antwika::input::Key kQuitKey = antwika::input::Key::Escape;

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
        logger.log(
            Level::Info,
            "Antwika Game on backends: " + std::string(backend->name())
                + " / " + std::string(inputBackend->name()));

        const auto window = backend->createWindow(
            WindowDesc{.title = "Antwika Game", .size = kWindowSize});

        // Opening the file is the application's job, not the library's.
        // antwika::gfx decodes bytes and never goes looking for them.
        std::ifstream atlasFile(
            ANTWIKA_GAME_ATLAS_PATH, std::ios::binary);
        const auto atlasBitmap = PngReader{}.read(atlasFile);

        // After the window, since a backend may have no device yet.
        // Declared after it too, so it is destroyed first.
        const auto atlas =
            window->renderer().createTexture(atlasBitmap);

        Camera camera(kInitialPan);
        PathIndex paths;
        const GridScene scene;

        // Against the size the window was asked for.
        // Never the size one reports, which nothing records.
        // That is what makes a recorded click hit the same button.
        UiOverlay overlay(kWindowSize);
        RenderSystem renderSystem(
            *window, scene, *atlas, paths, camera, kExtent, overlay);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        // Paced even under the backend that draws nothing.
        // That build used to stop after its scripted run.
        // An unbounded one would spin a core flat out instead.
        std::vector<std::reference_wrapper<ISystem>> observers{
            renderSystem, pacer};

        // Nothing is scripted unless a replay was asked for.
        // A plain run starts empty and builds only what gets clicked.
        std::vector<antwika::event::TickEvent> scripted;
        if (options.replayPath)
        {
            scripted =
                antwika::replay::loadReplayFile(*options.replayPath);
        }
        ReplaySource fileSource(std::move(scripted));

        // A --replay run must not attach a live source.
        // Every input would arrive twice: from the file and the device.
        const bool live = !options.replayPath.has_value();
        const InputEventCodec codec;
        LiveInputSource liveSource(fileSource, *inputBackend, codec);
        antwika::replay::IReplaySource &polled =
            live ? static_cast<antwika::replay::IReplaySource &>(liveSource)
                 : static_cast<antwika::replay::IReplaySource &>(fileSource);

        // Outside the live source, so they thin what the device reported.
        // Inside it, they would only ever have seen the scripted file.
        // In both branches, so a replay is thinned exactly as a run is.
        CoalescingPointerSource coalesced(polled);
        IdleMotionSource gated(coalesced, codec);

        // Both ways out are input, so both record and both replay.
        // A replay carrying its own stop simply gets a second one.
        // StopSignal ends the run on whichever arrives first.
        StopOnKeySource quitting(gated, codec, kQuitKey);
        WindowInputSource source(quitting, *backend, window->id());

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
            &replayRecorder,
            &overlay);

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
