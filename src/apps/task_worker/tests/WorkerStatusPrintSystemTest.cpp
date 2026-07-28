#include "antwika/task_worker/WorkerStatusPrintSystem.hpp"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task_worker/Worker.hpp"

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
using antwika::task_worker::makeWorkerLabel;
using antwika::task_worker::Worker;
using antwika::task_worker::WorkerStatus;
using antwika::task_worker::WorkerStatusPrintSystem;
using ::testing::NiceMock;

TEST(WorkerStatusPrintSystemTest, PrintsEachWorkersStatusAndRemainingTicks)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto idle = world.create();
    world.add<Worker>(idle, Worker{WorkerStatus::Idle, 0});
    const auto busy = world.create();
    world.add<Worker>(
        busy,
        Worker{WorkerStatus::Busy, 3, 42, makeWorkerLabel("Render")});
    world.commit();

    std::ostringstream out;
    WorkerStatusPrintSystem system(out);

    system.update(world, 5);

    EXPECT_EQ(
        out.str(),
        "After tick 5:\n"
        "  worker[0] - Current state: Idle\n"
        "  worker[1] - Current state: Busy | Remaining: 3 tick(s) | "
        "Task id: 42 | Task name: Render\n");
}
