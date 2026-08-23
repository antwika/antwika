#include "RaylibRenderTarget.hpp"

#include <rlgl.h>

#include <antwika/gfx/GfxError.hpp>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        [[nodiscard]] ::Texture2D wornAs(
            const unsigned int id, const Size size, const int format)
        {
            return ::Texture2D{
                .id = id,
                .width = static_cast<int>(size.width),
                .height = static_cast<int>(size.height),
                .mipmaps = 1,
                .format = format};
        }
    }

    RaylibRenderTarget::RaylibRenderTarget(
        RaylibRenderer &ownerRenderer, const RenderTargetSpec &spec)
        : owner(&ownerRenderer), targetSize(spec.size)
    {
        const auto width = static_cast<int>(spec.size.width);
        const auto height = static_cast<int>(spec.size.height);

        fbo = rlLoadFramebuffer();

        if (fbo == 0)
        {
            throw GfxError(
                "gfx.raylib: could not make a frame buffer");
        }

        rlEnableFramebuffer(fbo);

        colorId = rlLoadTexture(
            nullptr,
            width,
            height,
            RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
            1);

        rlFramebufferAttach(
            fbo,
            colorId,
            RL_ATTACHMENT_COLOR_CHANNEL0,
            RL_ATTACHMENT_TEXTURE2D,
            0);

        if (spec.depth)
        {
            depthId = rlLoadTextureDepth(width, height, false);

            rlFramebufferAttach(
                fbo,
                depthId,
                RL_ATTACHMENT_DEPTH,
                RL_ATTACHMENT_TEXTURE2D,
                0);
        }

        const bool complete = rlFramebufferComplete(fbo);

        rlDisableFramebuffer();

        if (!complete)
        {
            unload();

            throw GfxError(
                "gfx.raylib: the frame buffer will not hold "
                "together, which is what a driver that cannot hand "
                "depth back as a texture reports");
        }

        colorTexture = std::make_unique<RaylibTexture>(
            ownerRenderer,
            wornAs(
                colorId,
                spec.size,
                RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8),
            spec.size,
            false);

        if (spec.depth)
        {
            depthTexture = std::make_unique<RaylibTexture>(
                ownerRenderer,
                wornAs(
                    depthId,
                    spec.size,
                    RL_PIXELFORMAT_UNCOMPRESSED_R32),
                spec.size,
                false);
        }

        ownerRenderer.trackTarget(*this);
    }

    RaylibRenderTarget::~RaylibRenderTarget()
    {
        colorTexture.reset();
        depthTexture.reset();

        if (owner != nullptr)
        {
            owner->untrackTarget(*this);
        }

        unload();
    }

    void RaylibRenderTarget::unload() noexcept
    {
        if (!loaded)
        {
            return;
        }

        loaded = false;

        if (depthId != 0)
        {
            rlUnloadTexture(depthId);
        }

        if (colorId != 0)
        {
            rlUnloadTexture(colorId);
        }

        if (fbo != 0)
        {
            rlUnloadFramebuffer(fbo);
        }
    }

    Size RaylibRenderTarget::size() const
    {
        return targetSize;
    }

    const ITexture *RaylibRenderTarget::color() const
    {
        return colorTexture.get();
    }

    const ITexture *RaylibRenderTarget::depth() const
    {
        return depthTexture.get();
    }

    unsigned int RaylibRenderTarget::frameBuffer() const noexcept
    {
        return fbo;
    }

    bool RaylibRenderTarget::isOwnedBy(
        const RaylibRenderer &candidateRenderer) const noexcept
    {
        return owner == &candidateRenderer;
    }

    void RaylibRenderTarget::untrackRenderer() noexcept
    {
        owner = nullptr;
        loaded = false;

        if (colorTexture != nullptr)
        {
            colorTexture->untrackRenderer();
        }

        if (depthTexture != nullptr)
        {
            depthTexture->untrackRenderer();
        }
    }

}
