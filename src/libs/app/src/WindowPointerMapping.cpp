#include "antwika/app/WindowPointerMapping.hpp"

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Viewport.hpp>

namespace antwika::app
{

    WindowPointerMapping::WindowPointerMapping(
        const IWindow &window, Size canvas)
        : window(window), canvas(canvas)
    {
    }

    Position WindowPointerMapping::toSurface(Position position) const
    {
        const auto viewport =
            antwika::gfx::viewportFor(window.size(), canvas);

        const auto onCanvas = viewport.toCanvas(
            antwika::gfx::Point{.x = position.x, .y = position.y});

        return Position{.x = onCanvas.x, .y = onCanvas.y};
    }

} // namespace antwika::app
