#pragma once

#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/RenderTargetSpec.hpp>
#include <antwika/gfx/Size.hpp>

namespace antwika::gfx::detail
{

    class NullRenderTargetTexture final : public ITexture
    {
    public:
        explicit NullRenderTargetTexture(Size size) noexcept;

        [[nodiscard]] Size getSize() const override;

    private:
        Size extent;
    };

}
