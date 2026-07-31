#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    void PauseState::toggle() noexcept
    {
        held = !held;
    }

    bool PauseState::paused() const noexcept
    {
        return held;
    }

} // namespace antwika::game
