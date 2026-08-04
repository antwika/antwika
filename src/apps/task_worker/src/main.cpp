#include "antwika/task_worker/ConfigFile.hpp"
#include "antwika/task_worker/TaskWorker.hpp"

#include <chrono>
#include <iostream>

#include <antwika/app/AssetPath.hpp>
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
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/RenderSystem.hpp"
#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::app::WindowedSession;
using antwika::app::WindowedSessionDesc;
using antwika::log::Level;
using antwika::simulation::TickPacer;
using antwika::task_worker::PoolScene;
using antwika::task_worker::RenderSystem;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::Translator;
using antwika::time::SystemSleeper;

namespace
{

    // Wide enough for the pool beside the queue.
    // It is also what everything is laid out against.
    // Never the size the window reports back.
    constexpr antwika::gfx::Size kWindowSize{.width = 960, .height = 600};


    void run(const RecordedRun &recorded)
    {
        // The numbers the run reads off config.json, once.
        const auto config =
            antwika::task_worker::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);
        const auto inputBackend =
            antwika::input::makeSelectedInputBackend(logger);

        // Idle movement is held back and motion is coalesced.
        // Only the console reads the pointer here.
        // It hit-tests the folded position, not every point crossed.
        //
        // Closing the window ends the run, as in every windowed app.
        // It arrives as an engine.stop event through the source.
        // So closing one is recorded and replayed like anything else.
        const WindowedSessionDesc desc{
            .name = "Antwika TaskWorker",
            .windowTitle = "Antwika Task Worker",
            .canvas = kWindowSize,
            .input =
                {.coalescePointerMotion = true, .thinIdleMotion = true},
            .replayPath = recorded.options.replayPath,
            .demoReplay = antwika::app::assetPath("demo.jsonl")};

        WindowedSession session(logger, *backend, *inputBackend, desc);

        // Fixed here rather than read from anywhere.
        // A recording carries no locale of its own.
        // One from a flag would be missing from every replay.
        // Changing the language is this line, as the window size is.
        const Translator translator{antwika::i18n::kDefaultLocale};

        const PoolScene scene{translator};

        // The console's picture, against the configured size.
        // Never the size a window reports back.
        antwika::console::ConsolePicture consoleOverlay(session.canvas());

        TaskRegistry registry;
        RenderSystem renderSystem(
            session.window(), scene, registry, consoleOverlay);
        StatusPrintSystem printSystem(std::cout, registry);
        SystemSleeper sleeper;
        TickPacer pacer(
            sleeper,
            std::chrono::milliseconds(config.tickIntervalMs));

        antwika::task_worker::bootstrap(
            antwika::task_worker::TaskWorkerWiring{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = session.source(),
                .workerCount = config.workerCount,
                .observers = {printSystem, renderSystem, pacer},
                .registry = registry,
                .replayRecorder = recorded.replayRecorder,
                .consoleOverlay = consoleOverlay,
                .consoleLoadEnabled =
                    antwika::console::consoleLoadPermitted(
                        recorded.options.recordPath.has_value(),
                        recorded.options.replayPath.has_value())});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_task_worker", run);
}
