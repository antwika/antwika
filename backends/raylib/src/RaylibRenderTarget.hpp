#pragma once

#include <memory>

#include <raylib.h>

#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/RenderTargetSpec.hpp>
#include <antwika/gfx/Size.hpp>

#include "RaylibResource.hpp"
#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibRenderTarget final
        : public IRenderTarget,
          public RaylibResource
    {
    public:
        RaylibRenderTarget(
            RaylibRenderer &ownerRenderer, const RenderTargetSpec &spec);

        ~RaylibRenderTarget() override;

        [[nodiscard]] Size getSize() const override;

        [[nodiscard]] const ITexture *getColor() const override;

        [[nodiscard]] const ITexture *getDepth() const override;

        [[nodiscard]] unsigned int getFrameBuffer() const noexcept;

    private:
        void unloadHandle() noexcept override;

        Size targetSize;
        unsigned int fbo = 0;
        unsigned int colorId = 0;
        unsigned int depthId = 0;
        std::unique_ptr<RaylibTexture> colorTexture;
        std::unique_ptr<RaylibTexture> depthTexture;
    };

}
