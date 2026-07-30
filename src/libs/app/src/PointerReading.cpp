#include "antwika/app/PointerReading.hpp"

#include <optional>

namespace antwika::app
{

    Pointer pointerFrom(
        const InputState &state,
        bool located,
        MouseButton button) noexcept
    {
        const auto &mouse = state.mouse();
        return Pointer{
            .position = located
                            ? std::optional<Point>{asPoint(
                                  mouse.position())}
                            : std::nullopt,
            .down = mouse.isDown(button),
            .pressed = mouse.wasPressed(button)};
    }

} // namespace antwika::app
