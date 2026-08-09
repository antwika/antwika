#include "antwika/game/SessionGatedSystem.hpp"

namespace antwika::game
{

    SessionGatedSystem::SessionGatedSystem(
        ISystem &inner, const AppModeState &mode) noexcept
        : gate(inner, [&mode] { return simulates(mode.mode()); })
    {
    }

    void SessionGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

}
