#include "antwika/game/ModeGatedSystem.hpp"

namespace antwika::game
{

    ModeGatedSystem::ModeGatedSystem(
        ISystem &inner,
        const AppModeState &mode,
        AppMode active) noexcept
        : inner(inner), mode(mode), active(active)
    {
    }

    void ModeGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        if (mode.mode() == active)
        {
            inner.update(world, tick);
        }
    }

} // namespace antwika::game
