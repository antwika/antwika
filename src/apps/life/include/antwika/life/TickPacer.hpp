#pragma once

#include <chrono>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Waits a fixed interval every tick, so a run happens at a speed
     * somebody can watch.
     *
     * Registered as the last observer, after whatever draws the frame, which
     * makes the order present-then-wait -- what a frame limiter is.
     *
     * It touches neither World nor the tick it is given. Waiting changes
     * only how long a run takes, never what it computes, so a replay of a
     * paced run reproduces the same state as an unpaced one.
     */
    class TickPacer final : public ISystem
    {
    public:
        /**
         * @brief Construct the pacer over how long each tick should take.
         * @param interval How long to wait, once per tick.
         */
        explicit TickPacer(std::chrono::milliseconds interval);

        TickPacer(const TickPacer &) = delete;
        TickPacer(TickPacer &&) = delete;

        TickPacer &operator=(const TickPacer &) = delete;
        TickPacer &operator=(TickPacer &&) = delete;

        /**
         * @brief Wait out this tick's interval.
         * @param world Unused; never read and never written.
         * @param tick Unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::chrono::milliseconds interval;
    };

} // namespace antwika::life
