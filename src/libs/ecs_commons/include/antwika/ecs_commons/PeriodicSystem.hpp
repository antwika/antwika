#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Runs another system only on every nth tick.
     *
     * A decorator rather than a base class or a flag inside each system,
     * following life::DragPausedSystem: the wrapped system stays a plain
     * ISystem that knows nothing about cadence, and the scheduler still
     * sees exactly one system.
     * That is what lets the same system be run at two different rates in
     * two different phases without being written twice.
     *
     * Whether a tick is due is a pure function of the tick number, the
     * period and the offset -- there is no counter to keep, so a replay
     * that starts mid-run is due on exactly the ticks the original was,
     * and nothing has to be persisted for that to hold.
     *
     * Skipping means the inner system's update() is not called at all, so
     * nothing is staged.
     * The tick, the commit and every other system still run: this pauses
     * one system, not the simulation.
     */
    class PeriodicSystem final : public ISystem
    {
    public:
        /**
         * @brief Wrap a system in a cadence.
         * @param inner The system to run on due ticks. Must outlive this
         * object.
         * @param period How many ticks apart due ticks are. A period of
         * one is due every tick.
         * @param offset Which tick within each period is the due one,
         * taken modulo period. Defaults to zero, so tick zero is due.
         * @throws EcsCommonsError if period is zero, which would name no
         * ticks at all and silently disable the inner system.
         */
        PeriodicSystem(
            ISystem &inner,
            antwika::time::Tick period,
            antwika::time::Tick offset = 0);

        PeriodicSystem(const PeriodicSystem &) = delete;
        PeriodicSystem(PeriodicSystem &&) = delete;

        PeriodicSystem &operator=(const PeriodicSystem &) = delete;
        PeriodicSystem &operator=(PeriodicSystem &&) = delete;

        /**
         * @brief Run the inner system if this tick is due.
         * @param world The world to hand to the inner system.
         * @param tick The tick being processed; the only thing that
         * decides whether the inner system runs.
         */
        void update(World &world, antwika::time::Tick tick) override;

        /**
         * @brief Check whether a tick is one this cadence runs on.
         * @param tick The tick to ask about.
         * @return True when the inner system would run on that tick.
         */
        [[nodiscard]] bool due(antwika::time::Tick tick) const noexcept;

    private:
        ISystem &inner;
        antwika::time::Tick period;
        antwika::time::Tick offset;
    };

} // namespace antwika::ecs_commons
