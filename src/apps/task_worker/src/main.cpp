#include "antwika/task_worker/TaskWorker.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/simulation/WindowInputSource.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/RenderSystem.hpp"
#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::gfx::WindowDesc;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::simulation::TickPacer;
using antwika::simulation::WindowInputSource;
using antwika::task_worker::PoolScene;
using antwika::task_worker::RenderSystem;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskRegistry;
using antwika::task_worker::Translator;
using antwika::time::SystemSleeper;

namespace
{
    constexpr std::uint32_t kWorkerCount = 2;

    // Wide enough for the pool beside the queue.
    // It is also what everything is laid out against.
    // Never the size the window reports back.
    constexpr antwika::gfx::Size kWindowSize{.width = 960, .height = 600};

    // Slow enough to watch a queue drain, which the window is for.
    // The run is the same one either way.
    // A pacer changes how long a tick takes, never what it computes.
    constexpr std::chrono::milliseconds kTickInterval{400};

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        auto &logger = logging.logger();

        const auto backend = antwika::gfx::makeSelectedBackend(logger);

        logger.log(
            Level::Info,
            "Antwika TaskWorker on backend: "
                + std::string(backend->name()));

        const auto window = backend->createWindow(WindowDesc{
            .title = "Antwika Task Worker",
            .size = kWindowSize,
            .resizable = false});

        // Fixed here rather than read from anywhere.
        // A recording carries no locale of its own.
        // One from a flag would be missing from every replay.
        // Changing the language is this line, as the window size is.
        const Translator translator{antwika::i18n::kDefaultLocale};

        const PoolScene scene{translator};

        TaskRegistry registry;
        RenderSystem renderSystem(*window, scene, registry);
        StatusPrintSystem printSystem(std::cout, registry);
        SystemSleeper sleeper;
        TickPacer pacer(sleeper, kTickInterval);

        ReplaySource fileSource(antwika::app::scriptedEvents(
            recorded.options.replayPath,
            antwika::app::assetPath("demo.json")));

        // Closing the window ends the run, as in every windowed app.
        // It arrives as an engine.stop event through the source.
        // So closing one is recorded and replayed like anything else.
        WindowInputSource source(fileSource, *backend, window->id());

        antwika::task_worker::bootstrap(
            antwika::task_worker::TaskWorkerConfig{
                .logger = logger,
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .workerCount = kWorkerCount,
                .observers = {printSystem, renderSystem, pacer},
                .registry = registry,
                .replayRecorder = recorded.replayRecorder});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_task_worker", run);
}
