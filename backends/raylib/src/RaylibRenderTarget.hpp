#pragma once

#include <memory>

#include <raylib.h>

#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/RenderTargetSpec.hpp>
#include <antwika/gfx/Size.hpp>

#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

    class RaylibRenderer;

    class RaylibRenderTarget final : public IRenderTarget
    {
    public:
        RaylibRenderTarget(
            RaylibRenderer &ownerRenderer, const RenderTargetSpec &spec);

        RaylibRenderTarget(const RaylibRenderTarget &) = delete;
        RaylibRenderTarget(RaylibRenderTarget &&) = delete;

        RaylibRenderTarget &operator=(const RaylibRenderTarget &)
            = delete;
        RaylibRenderTarget &operator=(RaylibRenderTarget &&) = delete;

        ~RaylibRenderTarget() override;

        [[nodiscard]] Size size() const override;

        [[nodiscard]] const ITexture *color() const override;

        [[nodiscard]] const ITexture *depth() const override;

        [[nodiscard]] unsigned int frameBuffer() const noexcept;

        [[nodiscard]] bool isOwnedBy(
            const RaylibRenderer &candidateRenderer) const noexcept;

        void unload() noexcept;

        void untrackRenderer() noexcept;

    private:
        RaylibRenderer *owner;
        Size targetSize;
        unsigned int fbo = 0;
        unsigned int colorId = 0;
        unsigned int depthId = 0;
        std::unique_ptr<RaylibTexture> colorTexture;
        std::unique_ptr<RaylibTexture> depthTexture;
        bool loaded = true;
    };

}
