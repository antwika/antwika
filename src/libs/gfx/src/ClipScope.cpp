#include "antwika/gfx/ClipScope.hpp"

#include "antwika/gfx/ISurfaceRenderer.hpp"

namespace antwika::gfx
{

    ClipScope::ClipScope(ISurfaceRenderer &renderer) noexcept
        : renderer{renderer}
    {
    }

    ClipScope::~ClipScope()
    {
        renderer.endClip();
    }

}
