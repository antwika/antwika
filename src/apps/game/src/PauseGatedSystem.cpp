#include "antwika/game/PauseGatedSystem.hpp"

namespace antwika::game
{

    PauseGatedSystem::PauseGatedSystem(
        ISystem &inner, const PauseState &pause) noexcept
        : inner(inner), pause(pause)
    {
    }

    void PauseGatedSystem::update(World &world, antwika::time::Tick tick)
    {
        // Staging nothing is what holds the city still.
        // The commit after this phase then finds only what a click did.
        if (pause.paused())
        {
            return;
        }

        inner.update(world, tick);
    }

} // namespace antwika::game
