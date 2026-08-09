#pragma once

#include <ostream>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class StatusPrintSystem final : public ISystem
    {
    public:
        StatusPrintSystem(std::ostream &out, TaskRegistry &registry);

        StatusPrintSystem(const StatusPrintSystem &) = delete;
        StatusPrintSystem(StatusPrintSystem &&) = delete;

        StatusPrintSystem &operator=(const StatusPrintSystem &) = delete;
        StatusPrintSystem &operator=(StatusPrintSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::ostream &out;
        TaskRegistry &registry;
    };

}
