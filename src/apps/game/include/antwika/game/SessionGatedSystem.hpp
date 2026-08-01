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
     * @brief Runs a system only while there is a session on screen to
     * run.
     *
     * The simulation half of ModeGatedSink, and the same shape as
     * life::DragPausedSystem: what stops is the one system, while the
     * tick, the commit and every observer carry on -- so the menu is
     * still drawn and the run is still paced.
     *
     * **It gates on simulates() rather than on one named mode**, which
     * is what lets a city go on running while whoever is playing it
     * reads the world map.
     * Naming AppMode::CityMap alone is what used to stop it, and a city
     * that stopped because somebody opened a screen was a pause nobody
     * asked for -- see PauseState.
     *
     * This is what makes "the grid is not simulating behind the main
     * menu" true by construction rather than by there happening to be
     * nothing on the grid yet.
     */
    class SessionGatedSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the gate over the system it guards.
         * @param inner The system being guarded. Must outlive this gate.
         * @param mode The app's mode. Must outlive this gate.
         */
        SessionGatedSystem(
            ISystem &inner, const AppModeState &mode) noexcept;

        SessionGatedSystem(const SessionGatedSystem &) = delete;
        SessionGatedSystem(SessionGatedSystem &&) = delete;

        SessionGatedSystem &operator=(const SessionGatedSystem &) = delete;
        SessionGatedSystem &operator=(SessionGatedSystem &&) = delete;

        /**
         * @brief Run the inner system, or stage nothing at all.
         * @param world The world the inner system reads and stages into.
         * @param tick The tick being run.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        ISystem &inner;
        const AppModeState &mode;
    };

} // namespace antwika::game
