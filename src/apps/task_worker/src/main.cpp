#include "antwika/task_worker/TaskWorker.hpp"

#include <cstdint>
#include <iostream>

#include <antwika/app/AssetPath.hpp>
#include <antwika/app/ConsoleLogging.hpp>
#include <antwika/app/RunRecorded.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/task_worker/StatusPrintSystem.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

using antwika::app::ConsoleLogging;
using antwika::app::RecordedRun;
using antwika::log::Level;
using antwika::replay::ReplaySource;
using antwika::task_worker::StatusPrintSystem;
using antwika::task_worker::TaskRegistry;

namespace
{
    constexpr std::uint32_t kWorkerCount = 2;

    void run(const RecordedRun &recorded)
    {
        ConsoleLogging logging(std::cout, Level::Info);
        TaskRegistry registry;
        StatusPrintSystem printSystem(std::cout, registry);
        ReplaySource source(antwika::app::scriptedEvents(
            recorded.options.replayPath,
            antwika::app::assetPath("demo.json")));

        antwika::task_worker::bootstrap(
            antwika::task_worker::TaskWorkerConfig{
                .logger = logging.logger(),
                .eventSink = recorded.eventSink,
                .inputSource = source,
                .workerCount = kWorkerCount,
                .observers = {printSystem},
                .registry = registry,
                .replayRecorder = recorded.replayRecorder});
    }
} // namespace

int main(int argc, char **argv)
{
    return antwika::app::runRecorded(
        argc, argv, "antwika_task_worker", run);
}
