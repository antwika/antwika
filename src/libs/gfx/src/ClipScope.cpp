#include "antwika/gfx/ClipScope.hpp"

#include "antwika/gfx/IRenderer.hpp"

namespace antwika::gfx
{

    ClipScope::ClipScope(IRenderer &renderer) noexcept : renderer{renderer}
    {
    }

    ClipScope::~ClipScope()
    {
        renderer.endClip();
    }

}
