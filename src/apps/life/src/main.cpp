#include "antwika/life/Life.hpp"

#include <array>
#include <chrono>
#include <cstdint>
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
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/log/StreamAppender.hpp>
#include <antwika/replay/ReplayCli.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/SystemClock.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/life/BoardScene.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/PrintSystem.hpp"
#include "antwika/life/RenderSystem.hpp"
#include "antwika/life/TickPacer.hpp"
#include "antwika/life/WindowInputSource.hpp"

using antwika::ecs::ISystem;
using antwika::event::EventRecorder;
using antwika::event::TickEventRecorder;
using antwika::gfx::WindowDesc;
using antwika::life::BoardScene;
using antwika::life::PrintSystem;
using antwika::life::RenderSystem;
using antwika::life::TickPacer;
using antwika::life::WindowInputSource;
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
    constexpr std::uint32_t kBoardWidth = 32;
    constexpr std::uint32_t kBoardHeight = 32;

    // Square, and a whole number of pixels per cell at this board size.
    constexpr antwika::gfx::Size kWindowSize{.width = 768, .height = 768};

    constexpr std::chrono::milliseconds kTickInterval{50};

    // The backend that draws nothing.
    // A build using it has nothing to watch and nothing to wait for.
    constexpr std::string_view kHeadlessBackendName = "null";

    constexpr std::string_view kDemoReplayPath = ANTWIKA_LIFE_DEMO_REPLAY_PATH;

    constexpr std::array<std::string_view, 1> kSelfGeneratedEventNames{
        antwika::life::events::kStarted,
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
    EventRecorder eventSink;
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

        logger.log(
            Level::Info,
            "Antwika Life on backend: " + std::string(backend->name()));

        const auto window = backend->createWindow(
            WindowDesc{.title = "Antwika Life", .size = kWindowSize});

        const BoardScene scene;
        RenderSystem renderSystem(
            *window, scene, kBoardWidth, kBoardHeight);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        // A backend showing nothing leaves the board to be printed.
        // It also gives nobody a reason to wait between generations.
        std::vector<std::reference_wrapper<ISystem>> observers{renderSystem};
        if (showsNothing)
        {
            observers.emplace_back(printSystem);
        }
        else
        {
            observers.emplace_back(pacer);
        }

        auto events = antwika::replay::loadReplayFile(
            options.replayPath.value_or(std::string(kDemoReplayPath)));
        ReplaySource fileSource(std::move(events));
        WindowInputSource source(fileSource, *backend, window->id());

        antwika::life::bootstrap(
            logger,
            eventSink,
            source,
            kBoardWidth,
            kBoardHeight,
            observers,
            std::nullopt,
            &replayRecorder);
    }
    catch (const std::exception &error)
    {
        std::cerr << "antwika_life: " << error.what() << '\n';
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
