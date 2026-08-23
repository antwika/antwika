#include "antwika/gfx/TargetScope.hpp"

#include "antwika/gfx/IRenderer.hpp"

namespace antwika::gfx
{

    TargetScope::TargetScope(IRenderer &renderer) noexcept
        : renderer{renderer}
    {
    }

    TargetScope::~TargetScope()
    {
        renderer.endTarget();
    }

}
