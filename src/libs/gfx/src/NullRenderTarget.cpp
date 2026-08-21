#include "NullRenderTarget.hpp"

namespace antwika::gfx::detail
{

    NullRenderTargetTexture::NullRenderTargetTexture(
        const Size size) noexcept
        : extent(size)
    {
    }

    Size NullRenderTargetTexture::size() const
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

    Size NullRenderTarget::size() const
    {
        return extent;
    }

    const ITexture *NullRenderTarget::color() const
    {
        return &colorTexture;
    }

    const ITexture *NullRenderTarget::depth() const
    {
        return keepsDepth ? &depthTexture : nullptr;
    }

}
