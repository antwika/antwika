#pragma once

#include <cstdint>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs_commons/Name.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs
{
    class World;
} // namespace antwika::ecs

namespace antwika::task_worker
{

    /**
     * @brief Whether a Worker is free to claim a task, or busy on one.
     */
    enum class WorkerStatus : std::uint8_t
    {
        Idle,
        Busy,
    };

    /**
     * @brief Plain ECS component: a worker's current status and, when
     * Busy, how many more ticks it stays busy and which task it's
     * running. taskId/label are only meaningful while Busy; an Idle
     * worker always carries the defaults (0, "").
     *
     * label is an antwika::ecs_commons::Name rather than a std::string:
     * antwika::ecs::Component requires components stay trivially
     * copyable and standard-layout, which std::string isn't, and a
     * fixed-capacity label is exactly the content that library holds.
     * Build one with makeName() and read it back with view() -- a Name
     * that fills its buffer has no terminator, so label.text.data() is
     * not a C string and may not be handed to anything looking for one.
     */
    struct Worker
    {
        WorkerStatus status{WorkerStatus::Idle};
        antwika::time::Tick remainingTicks{0};
        std::uint64_t taskId{0};
        antwika::ecs_commons::Name label{};

        bool operator==(const Worker &other) const = default;
    };

    /**
     * @brief Snapshot every entity that currently has a Worker.
     * @param world World to query.
     * @return Every Worker entity, in View's insertion-stable order.
     *
     * The sole caller of World::view<Worker>().
     * WorkerCompletionSystem and StatusPrintSystem both need every
     * Worker entity.
     * Routing both through here keeps View<Worker> instantiated in
     * one place, rather than once per silently-duplicated caller.
     */
    [[nodiscard]] std::vector<antwika::ecs::Entity> allWorkers(
        const antwika::ecs::World &world);

} // namespace antwika::task_worker
