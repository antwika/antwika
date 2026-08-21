#include "RaylibRenderer.hpp"

#include <raylib.h>
#include <rlgl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/GfxError.hpp>

#include "RaylibFrame.hpp"
#include "RaylibMaterial.hpp"
#include "RaylibMesh.hpp"
#include "RaylibRenderTarget.hpp"
#include "RaylibRendererDetail.hpp"
#include "RaylibShader.hpp"
#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        constexpr std::size_t kMaxTransformDepth = 32;
    }

    RaylibRenderer::RaylibRenderer() = default;

    RaylibRenderer::~RaylibRenderer() = default;

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

        if (!clipRects.empty())
        {
            const auto &clip = clipRects.back();

            BeginScissorMode(
                static_cast<int>(clip.originPoint.x),
                static_cast<int>(clip.originPoint.y),
                static_cast<int>(clip.size.width),
                static_cast<int>(clip.size.height));
        }
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

        if (screenImage.data == nullptr || screenImage.width <= 0
            || screenImage.height <= 0)
        {
            UnloadImage(screenImage);

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

        UnloadImage(screenImage);

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
        for (RaylibRenderTarget *target : liveTargets)
        {
            target->unload();
            target->untrackRenderer();
        }

        liveTargets.clear();

        for (RaylibTexture *texture : liveTextures)
        {
            if (texture->isOwned())
            {
                UnloadTexture(texture->raw());
            }

            texture->untrackRenderer();
        }

        liveTextures.clear();

        for (RaylibMesh *mesh : liveMeshes)
        {
            UnloadMesh(mesh->raw());
            mesh->untrackRenderer();
        }

        liveMeshes.clear();

        for (RaylibShader *shader : liveShaders)
        {
            UnloadShader(shader->raw());
            shader->untrackRenderer();
        }

        liveShaders.clear();
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
