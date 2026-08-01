#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/ecs/World.hpp>
#include <antwika/scheduler/Priority.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"
#include "antwika/task_worker/Worker.hpp"

namespace antwika::task_worker
{

    /**
     * @brief One worker as a frame sees it: whether it is holding a task,
     * which one, and how far through it is.
     *
     * durationTicks is the task's whole length and remainingTicks what is
     * left of it, so a bar is (durationTicks - remainingTicks) of
     * durationTicks. Both are zero for an idle worker, and durationTicks
     * is zero for a busy one whose task no submit() call described --
     * which is a job scheduled past the registry rather than a state the
     * application can reach.
     */
    struct WorkerView
    {
        WorkerStatus status{WorkerStatus::Idle};
        std::uint64_t taskId{0};
        std::string label;
        antwika::time::Tick durationTicks{0};
        antwika::time::Tick remainingTicks{0};

        bool operator==(const WorkerView &other) const = default;
    };

    /**
     * @brief One task as a frame sees it, in a queue or in the list of
     * finished ones.
     *
     * blocked says the task is waiting on another rather than on a
     * worker, and waitingFor names that other task; both are false and
     * empty otherwise. A blocked task is not a candidate for
     * antwika::scheduler::Scheduler::run() at all, which is why it is
     * drawn apart from the ones that are.
     */
    struct TaskView
    {
        std::uint64_t taskId{0};
        std::string label;
        antwika::scheduler::Priority priority{};
        antwika::time::Tick durationTicks{0};
        bool blocked{false};
        std::string waitingFor;

        bool operator==(const TaskView &other) const = default;
    };

    /**
     * @brief Everything one frame draws, and nothing that can change it.
     *
     * A value taken on the tick, so drawing stays the write-only
     * projection the rest of this project keeps it as: a scene handed
     * one of these has no World, no TaskRegistry and no Scheduler to
     * write back to.
     *
     * It is not a second copy of anything either -- one is built afresh
     * from the World and the registry every tick and thrown away when
     * the frame is drawn, so there is nothing here to fall out of step
     * with the run.
     */
    struct PoolSnapshot
    {
        /** @brief The tick this picture is of. */
        antwika::time::Tick tick{0};

        /** @brief What the last dispatch was allowed to start, and did. */
        DispatchInfo dispatch{};

        /** @brief Every worker, in claim-priority order. */
        std::vector<WorkerView> workers;

        /**
         * @brief Every task still waiting, in the order the scheduler
         * will pull them.
         *
         * Ready tasks first, highest priority first and equal priorities
         * in submission order -- which is the comparator
         * antwika::scheduler::Scheduler sorts its own ready list with,
         * since a registry entry's index is its JobId less one. Blocked
         * tasks follow in submission order, because none of them is a
         * candidate until whatever it waits on has run.
         */
        std::vector<TaskView> queue;

        /** @brief Every finished task, in submission order. */
        std::vector<TaskView> completed;

        bool operator==(const PoolSnapshot &other) const = default;
    };

    /**
     * @brief Take this tick's picture of the pool and its queue.
     * @param world World the Worker entities live in; only read.
     * @param registry What every task's status is read from.
     * @param tick The tick the picture is of.
     * @return The snapshot.
     */
    [[nodiscard]] PoolSnapshot snapshotOf(
        const antwika::ecs::World &world,
        const TaskRegistry &registry,
        antwika::time::Tick tick);

} // namespace antwika::task_worker
