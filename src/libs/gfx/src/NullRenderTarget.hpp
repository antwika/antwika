#pragma once

#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/RenderTargetSpec.hpp>
#include <antwika/gfx/Size.hpp>

#include "NullRenderTargetTexture.hpp"

namespace antwika::gfx::detail
{

    class NullRenderTarget final : public IRenderTarget
    {
    public:
        explicit NullRenderTarget(const RenderTargetSpec &spec);

        [[nodiscard]] Size size() const override;

        [[nodiscard]] const ITexture *color() const override;

        [[nodiscard]] const ITexture *depth() const override;

    private:
        Size extent;
        NullRenderTargetTexture colorTexture;
        NullRenderTargetTexture depthTexture;
        bool keepsDepth;
    };

}
