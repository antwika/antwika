#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/BuildingIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Runs the economy: deliveries, drain, risk and demolition.
     *
     * Registered after WalkerSystem and before SpawnSystem, so a walker
     * delivers where this tick left it rather than where the last one
     * did, and a building freed by a demolition is not re-let the same
     * tick.
     *
     * **Deliveries are resolved here rather than in WalkerSystem**,
     * because what a walker is standing next to is a fact about the
     * buildings and not about the walking; putting it in the mover
     * would make one system need both views.
     *
     * A walker hands stock to every building orthogonally beside the
     * road it is on, in Direction order, until it runs out. A fireman
     * or an architect carries nothing and takes kRiskRelief off each
     * instead.
     *
     * **Two walkers delivering to one building in one tick is well
     * defined and it is not "both".** Both read the same committed
     * amount and both stage a write, so the last write wins -- which is
     * the ordinary double-buffer answer and would quietly halve a
     * delivery. So this accumulates every change to a building in a map
     * first and writes each building once, which makes the sum the
     * answer rather than the race.
     */
    class BuildingSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over what it reads and clears.
         * @param built Cleared as a building is demolished; must
         * outlive this system.
         */
        explicit BuildingSystem(BuildingIndex &built);

        BuildingSystem(const BuildingSystem &) = delete;
        BuildingSystem(BuildingSystem &&) = delete;

        BuildingSystem &operator=(const BuildingSystem &) = delete;
        BuildingSystem &operator=(BuildingSystem &&) = delete;

        /**
         * @brief Deliver, drain, raise risk, and demolish what is lost.
         *
         * A building at kMaxRisk, or a house that has run out of any
         * resource, is destroyed. Its walker is *not* destroyed with
         * it: it carries on until its own budget is spent, at which
         * point WalkerSystem finds no home to path to and removes it.
         *
         * @param world The world to read from and stage changes into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        BuildingIndex &built;
    };

} // namespace antwika::game
