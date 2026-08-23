#include "antwika/app/PointerReading.hpp"

#include <optional>

namespace antwika::app
{

    Pointer pointerFrom(
        const InputState &state,
        bool located,
        MouseButton button) noexcept
    {
        const auto &mouse = state.getMouse();
        return Pointer{
            .positionPoint = located
                            ? std::optional<Point>{getAsPoint(
                                  mouse.getPosition())}
                            : std::nullopt,
            .down = mouse.isDown(button),
            .pressed = mouse.wasPressed(button)};
    }

}
