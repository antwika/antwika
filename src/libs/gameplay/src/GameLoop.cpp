#include "antwika/gameplay/GameLoop.hpp"

#include <antwika/enums/Enumeration.hpp>

namespace antwika::gameplay
{

    std::string_view getPhaseName(const Phase phase)
    {
        switch (phase)
        {
        case Phase::Spawning:
            return "spawning";
        case Phase::Sending:
            return "sending";
        case Phase::Walking:
            return "walking";
        case Phase::Pickup:
            return "pickup";
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
                scheduler.createPhase(getPhaseName(phase));
        }
    }

    ecs::World &GameLoop::getWorld() noexcept
    {
        return *worldValue;
    }

    const ecs::World &GameLoop::getWorld() const noexcept
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
