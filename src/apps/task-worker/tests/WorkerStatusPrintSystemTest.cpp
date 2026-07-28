#include "antwika/task-worker/WorkerStatusPrintSystem.hpp"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/task-worker/Worker.hpp"

using antwika::ecs::World;
using antwika::log::mocks::MockLogger;
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
    world.add<Worker>(busy, Worker{WorkerStatus::Busy, 3});
    world.commit();

    std::ostringstream out;
    WorkerStatusPrintSystem system(out);

    system.update(world, 5);

    EXPECT_EQ(
        out.str(),
        "After tick 5:\n"
        "  worker[0]: Idle remaining=0\n"
        "  worker[1]: Busy remaining=3\n");
}
