#include "antwika/game/PauseState.hpp"

namespace antwika::game
{

    void PauseState::set(bool paused) noexcept
    {
        held = paused;
    }

    bool PauseState::paused() const noexcept
    {
        return held;
    }

} // namespace antwika::game
