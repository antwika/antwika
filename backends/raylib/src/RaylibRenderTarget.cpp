#include "RaylibRenderTarget.hpp"

#include <rlgl.h>

#include <antwika/gfx/GfxError.hpp>

#include "RaylibRenderer.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        [[nodiscard]] ::Texture2D getWornAs(
            const unsigned int id, const Size size, const int format)
        {
            return ::Texture2D{
                .id = id,
                .width = static_cast<int>(size.width),
                .height = static_cast<int>(size.height),
                .mipmaps = 1,
                .format = format};
        }

        [[nodiscard]] unsigned int createAttachedColor(
            const unsigned int fbo, const int width, const int height)
        {
            const auto colorId = rlLoadTexture(
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

            return colorId;
        }
    }

    RaylibRenderTarget::RaylibRenderTarget(
        RaylibRenderer &ownerRenderer, const RenderTargetSpec &spec)
        : RaylibResource(ownerRenderer), targetSize(spec.size)
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

        const bool keepsDepth = spec.depth || spec.depthOnly;

        if (!spec.depthOnly)
        {
            colorId = createAttachedColor(fbo, width, height);
        }

        if (keepsDepth)
        {
            depthId = rlLoadTextureDepth(width, height, false);

            rlFramebufferAttach(
                fbo,
                depthId,
                RL_ATTACHMENT_DEPTH,
                RL_ATTACHMENT_TEXTURE2D,
                0);
        }

        bool complete = rlFramebufferComplete(fbo);

        if (!complete && colorId == 0) // GCOVR_EXCL_START
        {
            colorId = createAttachedColor(fbo, width, height);
            complete = rlFramebufferComplete(fbo);
        } // GCOVR_EXCL_STOP

        rlDisableFramebuffer();

        if (!complete)
        {
            unload();

            throw GfxError(
                "gfx.raylib: the frame buffer will not hold "
                "together, which is what a driver that cannot hand "
                "depth back as a texture reports");
        }

        if (!spec.depthOnly)
        {
            colorTexture = std::make_unique<RaylibTexture>(
                ownerRenderer,
                getWornAs(
                    colorId,
                    spec.size,
                    RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8),
                spec.size,
                false);
        }

        if (keepsDepth)
        {
            depthTexture = std::make_unique<RaylibTexture>(
                ownerRenderer,
                getWornAs(
                    depthId,
                    spec.size,
                    RL_PIXELFORMAT_UNCOMPRESSED_R32),
                spec.size,
                false);
        }
    }

    RaylibRenderTarget::~RaylibRenderTarget()
    {
        unload();
    }

    void RaylibRenderTarget::unloadHandle() noexcept
    {
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

    Size RaylibRenderTarget::getSize() const
    {
        return targetSize;
    }

    const ITexture *RaylibRenderTarget::getColor() const
    {
        return colorTexture.get();
    }

    const ITexture *RaylibRenderTarget::getDepth() const
    {
        return depthTexture.get();
    }

    unsigned int RaylibRenderTarget::getFrameBuffer() const noexcept
    {
        return fbo;
    }

}
