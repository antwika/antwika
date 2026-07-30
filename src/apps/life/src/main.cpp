#include "antwika/life/Life.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/TickEventRecorder.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/input/IdleMotionSource.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/LiveInputSource.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/life/BoardScene.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/PrintSystem.hpp"
#include "antwika/life/RenderSystem.hpp"
#include "antwika/life/TickPacer.hpp"
#include "antwika/life/WindowInputSource.hpp"

using antwika::ecs::ISystem;
using antwika::ecs::World;
using antwika::event::TickEventRecorder;
using antwika::gfx::WindowDesc;
using antwika::input::IdleMotionSource;
using antwika::input::InputEventCodec;
using antwika::input::LiveInputSource;
using antwika::life::BoardScene;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::life::PrintSystem;
using antwika::life::RenderSystem;
using antwika::life::TickPacer;
using antwika::life::WindowInputSource;
using antwika::log::Level;
using antwika::log::Logger;
using antwika::log::MinimumLevelLogPolicy;
using antwika::log::PlainFormatter;
using antwika::log::StreamAppender;
using antwika::replay::IReplaySource;
using antwika::replay::ReplaySource;
using antwika::time::SystemClock;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::uint32_t kBoardWidth = 32;
    constexpr std::uint32_t kBoardHeight = 32;

    // Square, and a whole number of pixels per cell at this board size.
    // Also what a click is mapped against, not the size a window reports.
    // PointerToggleSink says why, and why the window is not resizable.
    constexpr antwika::gfx::Size kWindowSize{.width = 768, .height = 768};

    constexpr std::chrono::milliseconds kTickInterval{50};

    // The backend that draws nothing.
    // A build using it has nothing to watch and nothing to wait for.
    constexpr std::string_view kHeadlessBackendName = "null";

    constexpr std::string_view kDemoReplayPath = ANTWIKA_LIFE_DEMO_REPLAY_PATH;

    /**
     * @brief Sink for the events nothing in this app reads.
     *
     * Every dispatched event has to go somewhere, and an EventRecorder
     * used to be what went there -- deep-copying both strings of every
     * event and keeping them for the life of the process, in a run that
     * ends only when somebody closes the window.
     * Nothing ever called getEvents() on it.
     */
    class DiscardedEvents final : public antwika::event::IEventSink
    {
    public:
        void handle(const antwika::event::Event &) override
        {
        }
    };

} // namespace

int main(int argc, char **argv)
{
    const auto options = antwika::replay::parseReplayCliOptions(argc, argv);

    SystemClock clock;
    StreamAppender appender(std::cout);
    PlainFormatter formatter;
    MinimumLevelLogPolicy logPolicy(Level::Info);
    Logger logger(formatter, logPolicy, clock, appender);
    DiscardedEvents eventSink;
    TickEventRecorder replayRecorder;
    PrintSystem printSystem(kBoardWidth, std::cout);

    // Catching is what makes the run's resources unwind at all.
    // An uncaught exception may call std::terminate without unwinding.
    // Catching here also lets a failed --record run save what it has.
    int exitCode = EXIT_SUCCESS;
    try
    {
        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const bool showsNothing = backend->name() == kHeadlessBackendName;

        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        logger.log(
            Level::Info,
            "Antwika Life on backend: " + std::string(backend->name())
                + ", input: " + std::string(inputBackend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Life",
            .size = kWindowSize,
            .resizable = false});

        const BoardScene scene;
        RenderSystem renderSystem(
            *window, scene, kBoardWidth, kBoardHeight);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        // A backend showing nothing leaves the board to be printed.
        // Either way the run is paced, and paced last, after the frame.
        // A run with no end of its own would otherwise go flat out.
        std::vector<std::reference_wrapper<ISystem>> observers{renderSystem};
        if (showsNothing)
        {
            observers.emplace_back(printSystem);
        }
        observers.emplace_back(pacer);

        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource fileSource(std::move(events));

        const InputEventCodec codec;

        // Live input is attached only when there is no replay to run.
        // A replay already holds the input it recorded.
        // Reading a device too would make every event arrive twice.
        IReplaySource *seeded = &fileSource;
        std::optional<LiveInputSource> liveSource;
        if (!options.replayPath)
        {
            liveSource.emplace(fileSource, *inputBackend, codec);
            seeded = &*liveSource;
        }

        // Movement with the button up toggles nothing.
        // So it is held back rather than recorded, unlike mid-drag.
        // CoalescingPointerSource deliberately does not join it here.
        // A drag toggles every cell it crosses.
        // Thinning a run of movement inside a tick would skip cells.
        IdleMotionSource gated(*seeded, codec);

        WindowInputSource source(gated, *backend, window->id());

        // The run has no end of its own any more.
        // It goes on until the window closes, or a replay says to stop.
        // A headless build reports neither, so Ctrl+C ends one.
        if (showsNothing)
        {
            logger.log(
                Level::Info,
                "Antwika Life: this backend has no window to close, so "
                "press Ctrl+C to stop");
        }

        antwika::life::bootstrap(
            logger,
            eventSink,
            source,
            kBoardWidth,
            kBoardHeight,
            observers,
            std::nullopt,
            // Registered only when there is a file to write.
            // A run with no end would otherwise keep every event, forever.
            options.recordPath ? &replayRecorder : nullptr,
            [&codec](World &world, const Grid &grid, DragState &drag)
            {
                return std::make_unique<PointerToggleSink>(
                    world, grid, codec, kWindowSize, drag);
            });
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_life: " << error.what() << '\n';
        exitCode = EXIT_FAILURE;
    }

    if (options.recordPath)
    {
        antwika::replay::saveReplayFile(
            replayRecorder.getEvents(), *options.recordPath);
    }

    return exitCode;
}
