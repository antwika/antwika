#include "antwika/life/DragState.hpp"

namespace antwika::life
{

    void DragState::begin() noexcept
    {
        dragging = true;
    }

    void DragState::end() noexcept
    {
        dragging = false;
    }

    bool DragState::inProgress() const noexcept
    {
        return dragging;
    }

} // namespace antwika::life
