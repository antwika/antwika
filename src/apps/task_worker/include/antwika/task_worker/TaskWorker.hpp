#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    class TaskWorker final
    {
    public:
        explicit TaskWorker(IEngine &engine, ILogger &logger);

        TaskWorker(const TaskWorker &) = delete;
        TaskWorker(TaskWorker &&) = delete;

        TaskWorker &operator=(const TaskWorker &) = delete;
        TaskWorker &operator=(TaskWorker &&) = delete;

        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    struct TaskWorkerWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        std::uint32_t workerCount;

        std::vector<std::reference_wrapper<ISystem>> observers = {};

        std::optional<std::reference_wrapper<TaskRegistry>> registry =
            std::nullopt;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";
    };

    struct TaskWorkerSummary final
    {
        std::vector<Worker> workers;

        std::vector<std::string> console;
    };

    TaskWorkerSummary bootstrap(const TaskWorkerWiring &config);

}
