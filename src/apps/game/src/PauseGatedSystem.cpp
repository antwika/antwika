#include "antwika/game/PauseGatedSystem.hpp"

namespace antwika::game
{

    PauseGatedSystem::PauseGatedSystem(
        ISystem &inner, const PauseState &pause) noexcept
        // Staging nothing is what holds the city still.
        // The commit after this phase then finds only what a click did.
        : gate(inner, [&pause] { return !pause.paused(); })
    {
    }

    void PauseGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

} // namespace antwika::game
