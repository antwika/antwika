#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/Phase.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs
{

    /**
     * @brief Runs systems in phases, committing the World between phases.
     *
     * Phase order is creation order; system order within a phase is
     * registration order — both plain std::vector append order, so
     * nothing about scheduling depends on hashing or any other
     * non-deterministic structure.
     *
     * Committing after each phase (rather than once per tick) is what
     * makes phase ordering mean something: a later phase's systems see
     * everything every earlier phase in the same tick wrote, while
     * systems within one phase never see each other's writes — see
     * World::commit().
     */
    class SystemScheduler final
    {
    public:
        /**
         * @brief Create a new phase, ordered after every existing one.
         * @param name Used only for diagnostics — not compared or
         * hashed, so it has no effect on scheduling.
         * @return A handle to pass to addSystem().
         */
        [[nodiscard]] PhaseId createPhase(std::string_view name);

        /**
         * @brief Register a system to run within a phase.
         * @param phase A handle returned by createPhase().
         * @param system The system to run, in registration order within
         * the phase. Must outlive the scheduler.
         * @throws EcsError if phase was not returned by createPhase().
         */
        void addSystem(PhaseId phase, ISystem &system);

        /**
         * @brief Run every phase, in creation order, committing world
         * once after each phase's systems have all run.
         * @param world The world to run systems against.
         * @param tick The tick being processed.
         */
        void run(World &world, antwika::time::Tick tick);

    private:
        struct Phase
        {
            std::string name;
            std::vector<ISystem *> systems;
        };

        std::vector<Phase> phases;
    };

} // namespace antwika::ecs
