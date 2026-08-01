#include "antwika/game/SessionGatedSystem.hpp"

namespace antwika::game
{

    SessionGatedSystem::SessionGatedSystem(
        ISystem &inner, const AppModeState &mode) noexcept
        : inner(inner), mode(mode)
    {
    }

    void SessionGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        if (simulates(mode.mode()))
        {
            inner.update(world, tick);
        }
    }

} // namespace antwika::game
