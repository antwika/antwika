#pragma once

#include <chrono>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::time::ISleeper;

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
     *
     * The third copy of this class, after apps/life's and the pacing
     * apps/poker does inside its render sink.
     * It belongs in antwika::replay as a PacedReplaySource, which
     * docs/history/input-plan.md names as the eventual home for pacing;
     * moving it touches two working apps and is its own commit.
     *
     * The waiting itself belongs to antwika::time::ISleeper, which is what
     * everything in this project paces through. This is only the
     * ecs::ISystem shape around it, so a test can assert what was asked for
     * rather than spending the time.
     */
    class TickPacer final : public ISystem
    {
    public:
        /**
         * @brief Construct the pacer over how it waits, and for how long.
         * @param sleeper Does the waiting; must outlive this object.
         * @param interval How long to wait, once per tick.
         */
        TickPacer(ISleeper &sleeper, std::chrono::milliseconds interval);

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
        ISleeper &sleeper;
        std::chrono::milliseconds interval;
    };

} // namespace antwika::game
