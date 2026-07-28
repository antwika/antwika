#pragma once

#include <ostream>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Prints every Worker's status as text, once per tick.
     *
     * An independent, ECS-shaped per-tick observer -- it only reads
     * World (via world.view<Worker>()), never writes it, and knows
     * nothing about WorkerCompletionSystem/TaskDispatchSystem or any
     * other system registered alongside it in the same "observe" phase.
     */
    class WorkerStatusPrintSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the output stream it writes.
         * @param out Stream every worker-status snapshot is printed to.
         * Must outlive this system.
         */
        explicit WorkerStatusPrintSystem(std::ostream &out);

        /**
         * @brief Print World's current Worker states as text.
         * @param world World read from; never written to.
         * @param tick The tick this snapshot is for, printed as a label.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::ostream &out;
    };

} // namespace antwika::task_worker
