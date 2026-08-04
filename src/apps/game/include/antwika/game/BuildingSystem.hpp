#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"

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
     * road it is on, in Direction order, until it runs out.
     *
     * **The fire and collapse risks only ever climb on their own, and
     * a visit is what puts them back.** age() adds one to each per
     * risk period, unconditionally; the relief pass zeroes a
     * building's fire risk while a fireman stands beside it and its
     * collapse risk while an engineer does. The risks used to answer
     * to Service::Safety and Service::Structure coverage instead,
     * which made the danger a function of a countdown nobody could
     * see; the two services left with that mechanic.
     *
     * **The disease risk is the one that still answers to a
     * service.** Medicine is a state a doctor's round confers --
     * Service::Health -- so age() steps it up while the medicine has
     * run out and back down while there is any. Nothing ends a
     * building over it yet.
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
         * @param extent The bounds a demolition's leavers search for a
         * vacancy and a gate over -- see Demolition.hpp.
         * @param config The periods and caps the rules run on; copied,
         * so no lifetime rule attaches to it.
         */
        BuildingSystem(
            BuildingIndex &built, GridExtent extent, GameConfig config);

        BuildingSystem(const BuildingSystem &) = delete;
        BuildingSystem(BuildingSystem &&) = delete;

        BuildingSystem &operator=(const BuildingSystem &) = delete;
        BuildingSystem &operator=(BuildingSystem &&) = delete;

        /**
         * @brief Deliver, drain, step risk, and end what is lost.
         *
         * A building whose fire or collapse risk reaches kMaxRisk is
         * lost. Its walker is *not* destroyed with it: it carries on
         * until its own budget is spent, at which point WalkerSystem
         * finds no home to path to and removes it.
         *
         * **An empty larder ends nothing here any more.** A house out
         * of food or water empties instead, one person per settler
         * period -- see PopulationSystem -- and what finally takes an
         * unserved building is its own risk running all the way up.
         *
         * @param world The world to read from and stage changes into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        BuildingIndex &built;
        GridExtent extent;
        GameConfig config;
    };

} // namespace antwika::game
