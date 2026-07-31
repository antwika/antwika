#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Runs a system only while the app is in one mode.
     *
     * The simulation half of ModeGatedSink, and the same shape as
     * life::DragPausedSystem: what stops is the one system, while the
     * tick, the commit and every observer carry on -- so the menu is
     * still drawn and the run is still paced.
     *
     * This is what makes "the grid is not simulating behind the menu"
     * true by construction rather than by there happening to be nothing
     * on the grid yet.
     */
    class ModeGatedSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the gate over the system it guards.
         * @param inner The system being guarded. Must outlive this gate.
         * @param mode The app's mode. Must outlive this gate.
         * @param active The mode in which the inner system runs.
         */
        ModeGatedSystem(
            ISystem &inner,
            const AppModeState &mode,
            AppMode active) noexcept;

        ModeGatedSystem(const ModeGatedSystem &) = delete;
        ModeGatedSystem(ModeGatedSystem &&) = delete;

        ModeGatedSystem &operator=(const ModeGatedSystem &) = delete;
        ModeGatedSystem &operator=(ModeGatedSystem &&) = delete;

        /**
         * @brief Run the inner system, or stage nothing at all.
         * @param world The world the inner system reads and stages into.
         * @param tick The tick being run.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        ISystem &inner;
        const AppModeState &mode;
        AppMode active;
    };

} // namespace antwika::game
