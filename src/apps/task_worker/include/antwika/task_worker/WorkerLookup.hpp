#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;

    class WorkerLookup final
    {
    public:
        WorkerLookup(World &world, std::vector<Entity> workers);

        WorkerLookup(const WorkerLookup &) = delete;
        WorkerLookup(WorkerLookup &&) = delete;

        WorkerLookup &operator=(const WorkerLookup &) = delete;
        WorkerLookup &operator=(WorkerLookup &&) = delete;

        void reset(std::vector<Entity> replacement);

        void refresh();

        [[nodiscard]] std::size_t idleCount() const noexcept;

        bool claimIdle(
            antwika::time::Tick durationTicks,
            std::uint64_t taskId,
            std::string_view label);

    private:
        World &world;
        std::vector<Entity> workers;
        std::vector<bool> idle;
    };

}
