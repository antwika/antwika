#include "antwika/game/SessionGatedSystem.hpp"

namespace antwika::game
{

    SessionGatedSystem::SessionGatedSystem(
        ISystem &inner, const AppModeState &mode) noexcept
        // A city runs whether or not anybody is looking at it.
        // So this asks what the mode simulates, not which one it is.
        : gate(inner, [&mode] { return simulates(mode.mode()); })
    {
    }

    void SessionGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

} // namespace antwika::game
