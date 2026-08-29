#include "RaylibRenderer.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/log/Level.hpp>

#include "RaylibFrame.hpp"
#include "RaylibMaterial.hpp"
#include "RaylibRendererDetail.hpp"
#include "RaylibResource.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        struct ImageCloser final
        {
            void operator()(::Image *image) const noexcept
            {
                UnloadImage(*image);
            }
        };

        using HeldImage = std::unique_ptr<::Image, ImageCloser>;
    }

    RaylibRenderer::RaylibRenderer(ILogger &loggerGiven)
        : logger(loggerGiven)
    {
    }

    void RaylibRenderer::sayRefused(const std::string_view what) const
    {
        logger.log(
            antwika::log::Level::Warning,
            std::string("gfx.raylib: ") + std::string(what));
    }

    RaylibRenderer::~RaylibRenderer()
    {
        detach();
    }

    void RaylibRenderer::clear(Color color)
    {
        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        ClearBackground(toRaylib(color));
    }

        void RaylibRenderer::beginClip(const RectF areaRect)
    {
        ensureDrawing();

        auto clip = areaRect;

        if (!clipRects.empty())
        {
            const auto &clipRect = clipRects.back();
            const auto left =
                std::max(clip.originPoint.x, clipRect.originPoint.x);
            const auto top =
                std::max(clip.originPoint.y, clipRect.originPoint.y);
            const auto right = std::min(
                clip.originPoint.x + clip.size.width,
                clipRect.originPoint.x + clipRect.size.width);
            const auto bottom = std::min(
                clip.originPoint.y + clip.size.height,
                clipRect.originPoint.y + clipRect.size.height);

            clip = RectF(
                {left, top},
                {std::max(right - left, 0.0F),
                 std::max(bottom - top, 0.0F)});
        }

        clipRects.push_back(clip);

        if (drawing)
        {
            BeginScissorMode(
                static_cast<int>(clip.originPoint.x),
                static_cast<int>(clip.originPoint.y),
                static_cast<int>(clip.size.width),
                static_cast<int>(clip.size.height));
        }
    }

    void RaylibRenderer::endClip()
    {
        if (!clipRects.empty())
        {
            clipRects.pop_back();
        }

        if (!drawing)
        {
            return;
        }

        EndScissorMode();
        applyClipScissor();
    }

    void RaylibRenderer::applyClipScissor()
    {
        if (!drawing || clipRects.empty())
        {
            return;
        }

        const auto &clip = clipRects.back();

        BeginScissorMode(
            static_cast<int>(clip.originPoint.x),
            static_cast<int>(clip.originPoint.y),
            static_cast<int>(clip.size.width),
            static_cast<int>(clip.size.height));
    }

    void RaylibRenderer::pushTransform(const Mat4 &transform)
    {
        if (pushedCount == kMaxTransformDepth)
        {
            throw GfxError(
                "gfx.raylib: the transform stack is full");
        }

        ensureDrawing();

        ++pushedCount;

        if (!drawing)
        {
            return;
        }

        rlPushMatrix();
        rlMultMatrixf(&transform[0][0]);
    }

    void RaylibRenderer::popTransform()
    {
        if (pushedCount == 0)
        {
            throw GfxError("gfx.raylib: no transform is pushed");
        }

        --pushedCount;

        if (!drawing)
        {
            return;
        }

        rlPopMatrix();
    }

    Bitmap RaylibRenderer::readPixels()
    {
        ensureDrawing();

        if (!drawing)
        {
            return Bitmap{};
        }

        rlDrawRenderBatchActive();

        ::Image screenImage = LoadImageFromScreen();
        const HeldImage heldImage{&screenImage};

        if (screenImage.data == nullptr || screenImage.width <= 0
            || screenImage.height <= 0)
        {
            return Bitmap{};
        }

        ImageFormat(&screenImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        const auto width = static_cast<std::uint32_t>(screenImage.width);
        const auto height = static_cast<std::uint32_t>(screenImage.height);

        Bitmap takenBitmap{
            .size = {.width = width, .height = height},
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(width) * height
                * kBytesPerPixel)};

        const auto *source =
            static_cast<const std::uint8_t *>(screenImage.data);

        std::copy_n(
            source,
            takenBitmap.pixels.size(),
            takenBitmap.pixels.begin());

        return takenBitmap;
    }

    void RaylibRenderer::present()
    {
        if (!drawing)
        {
            return;
        }

        while (pushedCount > 0)
        {
            rlPopMatrix();
            --pushedCount;
        }

        EndDrawing();
        drawing = false;

        antwika::raylib::advanceFrame();
    }

    void RaylibRenderer::detach()
    {
        if (!attached)
        {
            return;
        }

        for (RaylibResource *resource : liveResources)
        {
            resource->unload();
            resource->untrackRenderer();
        }

        liveResources.clear();
        uniformLocations.clear();

        material.reset();

        present();
        attached = false;
    }

    void RaylibRenderer::ensureDrawing()
    {
        if (drawing || !attached)
        {
            return;
        }

        BeginDrawing();
        drawing = true;
    }

}
