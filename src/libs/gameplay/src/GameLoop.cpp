#include "antwika/gameplay/GameLoop.hpp"

#include <antwika/enums/Enumeration.hpp>

namespace antwika::gameplay
{

    std::string_view phaseName(const Phase phase)
    {
        switch (phase)
        {
        case Phase::Sending:
            return "sending";
        case Phase::Walking:
            return "walking";
        case Phase::Orienting:
            return "gazing";
        case Phase::Health:
            break;
        }

        return "health";
    }

    GameLoop::GameLoop(ecs::World &world) : worldValue(&world)
    {
        for (const auto phase : kAllPhases)
        {
            phases.at(enums::index(phase)) =
                scheduler.createPhase(phaseName(phase));
        }
    }

    ecs::World &GameLoop::world() noexcept
    {
        return *worldValue;
    }

    const ecs::World &GameLoop::world() const noexcept
    {
        return *worldValue;
    }

    void GameLoop::addSystem(const Phase phase, ecs::ISystem &system)
    {
        scheduler.addSystem(phases.at(enums::index(phase)), system);
    }

    void GameLoop::run(const time::Tick tick)
    {
        scheduler.run(*worldValue, tick);
    }

}
