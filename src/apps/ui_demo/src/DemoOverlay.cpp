#include "antwika/ui_demo/DemoOverlay.hpp"

#include <utility>

namespace antwika::ui_demo
{

    DemoOverlay::DemoOverlay(const Size canvas) : area(canvas)
    {
    }

    Size DemoOverlay::canvas() const noexcept
    {
        return area;
    }

    void DemoOverlay::set(DrawList commands)
    {
        picture = std::move(commands);
    }

    const DrawList &DemoOverlay::commands() const noexcept
    {
        return picture;
    }

}
