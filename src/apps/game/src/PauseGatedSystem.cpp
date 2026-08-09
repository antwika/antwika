#include "antwika/game/PauseGatedSystem.hpp"

namespace antwika::game
{

    PauseGatedSystem::PauseGatedSystem(
        ISystem &inner, const PauseState &pause) noexcept
        : gate(inner, [&pause] { return !pause.paused(); })
    {
    }

    void PauseGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

}
