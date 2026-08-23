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
          keepsDepth(spec.depth)
    {
    }

    Size NullRenderTarget::getSize() const
    {
        return extent;
    }

    const ITexture *NullRenderTarget::getColor() const
    {
        return &colorTexture;
    }

    const ITexture *NullRenderTarget::getDepth() const
    {
        return keepsDepth ? &depthTexture : nullptr;
    }

}
