#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include <antwika/console/SnapshotError.hpp>

#include "antwika/task_worker/StateDump.hpp"

using antwika::console::SnapshotError;
using antwika::task_worker::DispatchInfo;
using antwika::task_worker::StateDump;
using antwika::task_worker::SubmissionDump;
using antwika::task_worker::TaskDependency;
using antwika::task_worker::TaskInfo;
using antwika::task_worker::TaskStatus;
using antwika::task_worker::WorkerDump;
using antwika::task_worker::WorkerStatus;

namespace
{
    [[nodiscard]] StateDump fullDump()
    {
        StateDump dump;

        dump.workers.push_back(
            WorkerDump{WorkerStatus::Idle, 0, 0, ""});
        dump.workers.push_back(
            WorkerDump{WorkerStatus::Busy, 4, 2, "Beta"});

        dump.tasks.push_back(TaskInfo{
            1, "Alpha", antwika::scheduler::Priority{1},
            TaskStatus::Completed, 3, 0, std::nullopt});
        dump.tasks.push_back(TaskInfo{
            2, "Beta", antwika::scheduler::Priority{2},
            TaskStatus::Running, 6, 4, std::nullopt});
        dump.tasks.push_back(TaskInfo{
            3, "Gamma", antwika::scheduler::Priority{1},
            TaskStatus::Pending, 2, 2,
            TaskDependency{1, "Alpha"}});
        dump.tasks.push_back(TaskInfo{
            4, "Delta", antwika::scheduler::Priority{3},
            TaskStatus::Pending, 5, 5, std::nullopt});

        dump.submissions.push_back(SubmissionDump{1, "Alpha"});
        dump.submissions.push_back(SubmissionDump{2, "Beta"});
        dump.submissions.push_back(SubmissionDump{3, "Gamma"});
        dump.submissions.push_back(SubmissionDump{4, "Delta"});

        dump.dispatch = DispatchInfo{2, 1};

        return dump;
    }
}

TEST(StateDumpTest, StateDumpFromJson_KeepsStatusAndDependencies)
{
    const auto dump = fullDump();

    const auto decoded = antwika::task_worker::stateDumpFromJson(
        antwika::task_worker::stateDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
}

TEST(StateDumpTest, StateDumpFromJson_ResolvesADependencyLabel)
{
    const auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());

    EXPECT_FALSE(encoded["tasks"][2].contains("dependsOnLabel"));
    const auto decoded =
        antwika::task_worker::stateDumpFromJson(encoded);

    EXPECT_EQ(
        decoded.tasks[2].dependsOn,
        (std::optional<TaskDependency>{TaskDependency{1, "Alpha"}}));
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownWorkerStatus)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["workers"][0]["status"] = "sleeping";

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownTaskStatus)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["tasks"][0]["status"] = "paused";

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesANonNumericPriority)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["tasks"][0]["priority"] = "high";

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAPriorityPastAByte)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["tasks"][0]["priority"] = 256;

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownDependency)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["tasks"][2]["dependsOn"] = 999;

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAMissingMember)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded.erase("workers");

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAMissingTaskMember)
{
    auto encoded =
        antwika::task_worker::stateDumpToJson(fullDump());
    encoded["tasks"][0].erase("label");

    EXPECT_THROW(
        static_cast<void>(
            antwika::task_worker::stateDumpFromJson(encoded)),
        SnapshotError);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryWorkerDumpField)
{
    const WorkerDump base{WorkerStatus::Busy, 4, 2, "Beta"};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto idled = base;
    idled.status = WorkerStatus::Idle;
    EXPECT_NE(base, idled);

    auto later = base;
    later.remainingTicks = 9;
    EXPECT_NE(base, later);

    auto retasked = base;
    retasked.taskId = 7;
    EXPECT_NE(base, retasked);

    auto renamed = base;
    renamed.label = "Gamma";
    EXPECT_NE(base, renamed);
}

TEST(StateDumpTest, OperatorEquals_ComparesEverySubmissionDumpField)
{
    const SubmissionDump base{1, "Alpha"};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto renumbered = base;
    renumbered.taskId = 2;
    EXPECT_NE(base, renumbered);

    auto renamed = base;
    renamed.label = "Beta";
    EXPECT_NE(base, renamed);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryField)
{
    const auto base = fullDump();

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto reworked = base;
    reworked.workers.pop_back();
    EXPECT_NE(base, reworked);

    auto retasked = base;
    retasked.tasks.pop_back();
    EXPECT_NE(base, retasked);

    auto resubmitted = base;
    resubmitted.submissions.pop_back();
    EXPECT_NE(base, resubmitted);

    auto redispatched = base;
    redispatched.dispatch.dispatched += 1;
    EXPECT_NE(base, redispatched);
}
