#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Holds another system still while the run is paused.
     *
     * The same shape as ModeGatedSystem and life::DragPausedSystem, on a
     * different question: what stops is the one system it wraps, while
     * the tick, the commit and every observer carry on. So a paused city
     * is still drawn, the toolbar still answers, the camera still pans
     * and a building still goes up where it is clicked -- a pause nobody
     * could see would just look like a freeze.
     *
     * A decorator rather than a flag inside WalkerSystem and its two
     * neighbours, which keeps each rule unaware that anything could pause
     * it, and lets the same gate be put over any other system that should
     * stand still for the same reason.
     *
     * It composes with ModeGatedSystem rather than duplicating it: a run
     * is paused *and* in a mode, and either gate alone answers only its
     * own question. bootstrap() wraps the mode gate in this one, so the
     * two orders cannot be told apart -- neither stages anything unless
     * both agree.
     *
     * Pausing costs the run nothing in reproducibility: which ticks were
     * paused follows from the recorded presses, so a replay pauses on
     * exactly the same ones -- see PauseState.
     */
    class PauseGatedSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the gate over the system it guards.
         * @param inner The system being guarded. Must outlive this gate.
         * @param pause Consulted every tick. Must outlive this gate.
         */
        PauseGatedSystem(ISystem &inner, const PauseState &pause) noexcept;

        PauseGatedSystem(const PauseGatedSystem &) = delete;
        PauseGatedSystem(PauseGatedSystem &&) = delete;

        PauseGatedSystem &operator=(const PauseGatedSystem &) = delete;
        PauseGatedSystem &operator=(PauseGatedSystem &&) = delete;

        /**
         * @brief Run the inner system, or stage nothing at all.
         * @param world The world the inner system reads and stages into.
         * @param tick The tick being run.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        ISystem &inner;
        const PauseState &pause;
    };

} // namespace antwika::game
