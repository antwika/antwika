#include "antwika/gfx/TransformScope.hpp"

#include "antwika/gfx/IRenderer.hpp"

namespace antwika::gfx
{

    TransformScope::TransformScope(IRenderer &renderer) noexcept
        : renderer{renderer}
    {
    }

    TransformScope::~TransformScope()
    {
        renderer.popTransform();
    }

}
