#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    void PauseState::toggle() noexcept
    {
        held = !held;
    }

    void PauseState::hold() noexcept
    {
        held = true;
    }

    void PauseState::release() noexcept
    {
        held = false;
    }

    bool PauseState::paused() const noexcept
    {
        return held;
    }

} // namespace antwika::game
