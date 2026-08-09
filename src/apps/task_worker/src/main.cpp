#include <chrono>
#include <iostream>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/app/WindowedHost.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/console/SnapshotCommands.hpp>
#include <antwika/gfx/SelectedBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/input/SelectedInputBackend.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/time/SystemSleeper.hpp>

#include "antwika/task_worker/ConfigFile.hpp"
#include "antwika/task_worker/TaskWorker.hpp"
#include "antwika/task_worker/Messages.hpp"
#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/RenderSystem.hpp"
#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::app::RecordedRun;
using antwika::app::WindowedHost;
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

    constexpr antwika::gfx::Size kWindowSize{.width = 960, .height = 600};

    void run(const RecordedRun &recorded)
    {
        const auto config =
            antwika::task_worker::loadConfigFileOrDefaults(
                antwika::app::assetPath("config.json"));

        WindowedHost host(
            std::cout,
            Level::Info,
            {.gfx = antwika::gfx::makeSelectedBackend,
             .input = antwika::input::makeSelectedInputBackend},
            WindowedSessionDesc{
                .name = "Antwika TaskWorker",
                .windowTitle = "Antwika Task Worker",
                .canvas = kWindowSize,
                .input =
                    {.coalescePointerMotion = true, .thinIdleMotion = true},
                .replayPath = recorded.options.replayPath,
                .demoReplay = antwika::app::assetPath("demo.jsonl")});

        auto &logger = host.logger();
        auto &session = host.session();

        const Translator translator{antwika::i18n::kDefaultLocale};

        const PoolScene scene{translator};

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
                    antwika::console::consoleLoadPermitted(recorded.options)});
    }
}

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_task_worker", run);
}
