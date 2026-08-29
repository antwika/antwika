#include "NullRenderTarget.hpp"

namespace antwika::gfx::detail
{

    NullRenderTargetTexture::NullRenderTargetTexture(
        const Size size) noexcept
        : extent(size)
    {
    }

    Size NullRenderTargetTexture::getSize() const
    {
        return extent;
    }

    NullRenderTarget::NullRenderTarget(const RenderTargetSpec &spec)
        : extent(spec.size),
          colorTexture(spec.size),
          depthTexture(spec.size),
          keepsColor(!spec.depthOnly),
          keepsDepth(spec.depth || spec.depthOnly)
    {
    }

    Size NullRenderTarget::getSize() const
    {
        return extent;
    }

    const ITexture *NullRenderTarget::getColor() const
    {
        return keepsColor ? &colorTexture : nullptr;
    }

    const ITexture *NullRenderTarget::getDepth() const
    {
        return keepsDepth ? &depthTexture : nullptr;
    }

}
