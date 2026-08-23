#include <raylib.h>
#include <rlgl.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include <antwika/gfx/Blit.hpp>

#include "RaylibRenderer.hpp"
#include "RaylibMaterial.hpp"
#include "RaylibMesh.hpp"
#include "RaylibRenderTarget.hpp"
#include "RaylibRendererDetail.hpp"
#include "RaylibShader.hpp"
#include "RaylibTexture.hpp"

namespace antwika::gfx::raylib
{

void RaylibRenderer::drawRect(RectF rect, Color color)
    {
        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        DrawRectangleRec(
            ::Rectangle{
                .x = rect.originPoint.x,
                .y = rect.originPoint.y,
                .width = rect.size.width,
                .height = rect.size.height},
            toRaylib(color));
    }

    void RaylibRenderer::drawLine(PointF fromPoint, PointF toPoint, Color color)
    {
        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        const auto raylibColor = toRaylib(color);

        const ::Vector2 start{.x = fromPoint.x, .y = fromPoint.y};
        const ::Vector2 end{.x = toPoint.x, .y = toPoint.y};

        if (fromPoint == toPoint)
        {
            DrawPixelV(start, raylibColor);
            return;
        }

        DrawLineV(start, end, raylibColor);
    }

    void RaylibRenderer::drawText(
        PointF originPoint,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        glyphAtlases.draw(*this, originPoint, text, scale, color);
    }

    void RaylibRenderer::drawTexture(
        const ITexture &texture,
        RectF sourceRect,
        RectF destinationRect,
        Color tintColor)
    {
        const auto *mine = dynamic_cast<const RaylibTexture *>(&texture);

        if (mine == nullptr || !mine->isOwnedBy(*this) || !mine->isLoaded())
        {
            return;
        }

        if (!isBlitIsInBounds(mine->getSize(), sourceRect, destinationRect))
        {
            return;
        }

        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        const ::Rectangle sourceRectangle{
            .x = sourceRect.originPoint.x,
            .y = sourceRect.originPoint.y,
            .width = sourceRect.size.width,
            .height = sourceRect.size.height};

        const ::Rectangle destinationRectangle{
            .x = destinationRect.originPoint.x,
            .y = destinationRect.originPoint.y,
            .width = destinationRect.size.width,
            .height = destinationRect.size.height};

        DrawTexturePro(
            mine->getRawHandle(),
            sourceRectangle,
            destinationRectangle,
            ::Vector2{.x = 0.0F, .y = 0.0F},
            0.0F,
            toRaylib(tintColor));
    }

    int RaylibRenderer::uniformLocationOf(
        const IShader &shader,
        const ::Shader &nativeShader,
        const std::string_view name)
    {
        auto &knownLocations = uniformLocations[&shader];
        const auto foundEntry = knownLocations.find(name);

        if (foundEntry != knownLocations.end())
        {
            return foundEntry->second;
        }

        const std::string uniformName(name);
        const auto where = GetShaderLocation(nativeShader, uniformName.c_str());

        knownLocations.emplace(uniformName, where);

        return where;
    }

    void RaylibRenderer::setShaderValue(
        const IShader &shader,
        const std::string_view name,
        const void *value,
        const int kind)
    {
        const auto *mine = ownShaderOf(&shader);

        if (mine == nullptr)
        {
            return;
        }

        const auto where = uniformLocationOf(shader, *mine, name);

        if (where < 0)
        {
            return;
        }

        SetShaderValue(*mine, where, value, kind);
    }

    void RaylibRenderer::setShaderNumber(
        const IShader &shader,
        const std::string_view name,
        const float value)
    {
        setShaderValue(shader, name, &value, SHADER_UNIFORM_FLOAT);
    }

    void RaylibRenderer::setShaderVector(
        const IShader &shader,
        const std::string_view name,
        const Vec3 vector)
    {
        const std::array<float, 3> sentValues{vector.x, vector.y, vector.z};

        setShaderValue(
            shader, name, sentValues.data(), SHADER_UNIFORM_VEC3);
    }

    void RaylibRenderer::setShaderMatrix(
        const IShader &shader,
        const std::string_view name,
        const Mat4 &matrix)
    {
        const auto *mine = ownShaderOf(&shader);

        if (mine == nullptr)
        {
            return;
        }

        const auto where = uniformLocationOf(shader, *mine, name);

        if (where < 0)
        {
            return;
        }

        SetShaderValueMatrix(*mine, where, toRaylib(matrix));
    }

    void RaylibRenderer::setShaderColor(
        const IShader &shader,
        const std::string_view name,
        const Color valueColor)
    {
        const std::array<float, 4> sentValues{
            static_cast<float>(valueColor.red) / 255.0F,
            static_cast<float>(valueColor.green) / 255.0F,
            static_cast<float>(valueColor.blue) / 255.0F,
            static_cast<float>(valueColor.alpha) / 255.0F};

        setShaderValue(
            shader, name, sentValues.data(), SHADER_UNIFORM_VEC4);
    }

    namespace
    {
        void setOrthoProjection(const Size surfaceSize)
        {
            rlMatrixMode(RL_PROJECTION);
            rlLoadIdentity();
            rlOrtho(
                0.0,
                static_cast<double>(surfaceSize.width),
                static_cast<double>(surfaceSize.height),
                0.0,
                0.0,
                1.0);
            rlMatrixMode(RL_MODELVIEW);
            rlLoadIdentity();
        }
    }

    void RaylibRenderer::beginTarget(IRenderTarget &target)
    {
        auto *mine = dynamic_cast<RaylibRenderTarget *>(&target);

        if (mine == nullptr || !mine->isOwnedBy(*this)
            || inTarget != nullptr)
        {
            return;
        }

        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        rlDrawRenderBatchActive();

        inTarget = mine;

        rlEnableFramebuffer(mine->getFrameBuffer());
        rlViewport(
            0,
            0,
            static_cast<int>(mine->getSize().width),
            static_cast<int>(mine->getSize().height));
        rlSetFramebufferWidth(static_cast<int>(mine->getSize().width));
        rlSetFramebufferHeight(static_cast<int>(mine->getSize().height));
        setOrthoProjection(mine->getSize());
    }

    void RaylibRenderer::beginTargetRegion(
        IRenderTarget &target, const Rect regionRect)
    {
        auto *mine = dynamic_cast<RaylibRenderTarget *>(&target);

        if (mine == nullptr || !mine->isOwnedBy(*this)
            || inTarget != nullptr)
        {
            return;
        }

        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        rlDrawRenderBatchActive();

        inTarget = mine;

        rlEnableFramebuffer(mine->getFrameBuffer());

        inRegionRect = regionRect;

        rlSetFramebufferWidth(static_cast<int>(regionRect.size.width));
        rlSetFramebufferHeight(static_cast<int>(regionRect.size.height));

        applyRegionViewport();
        setOrthoProjection(regionRect.size);
    }

    void RaylibRenderer::applyRegionViewport()
    {
        if (inTarget == nullptr || !inRegionRect.has_value())
        {
            return;
        }

        const auto targetHeight =
            static_cast<int>(inTarget->getSize().height)
            - inRegionRect->originPoint.y
            - static_cast<int>(inRegionRect->size.height);

        rlViewport(
            inRegionRect->originPoint.x,
            targetHeight,
            static_cast<int>(inRegionRect->size.width),
            static_cast<int>(inRegionRect->size.height));
        rlEnableScissorTest();
        rlScissor(
            inRegionRect->originPoint.x,
            targetHeight,
            static_cast<int>(inRegionRect->size.width),
            static_cast<int>(inRegionRect->size.height));
    }

    void RaylibRenderer::endTarget()
    {
        if (inTarget == nullptr)
        {
            return;
        }

        rlDrawRenderBatchActive();
        rlDisableScissorTest();
        rlDisableFramebuffer();

        inTarget = nullptr;
        inRegionRect.reset();

        const auto width = GetScreenWidth();
        const auto height = GetScreenHeight();

        rlViewport(0, 0, width, height);
        rlSetFramebufferWidth(width);
        rlSetFramebufferHeight(height);
        setOrthoProjection(
            Size{
                .width = static_cast<std::uint32_t>(width),
                .height = static_cast<std::uint32_t>(height)});
    }

    void RaylibRenderer::drawMesh(
        const IMesh &mesh,
        const Mat4 &modelMatrix,
        const Camera3D &camera,
        const MeshMaterial &surfaceMaterial)
    {
        const auto *mine = dynamic_cast<const RaylibMesh *>(&mesh);

        if (mine == nullptr || !mine->isOwnedBy(*this) || !mine->isLoaded())
        {
            return;
        }

        ensureDrawing();

        if (!drawing)
        {
            return;
        }

        if (material == nullptr)
        {
            material = std::make_unique<RaylibMaterial>();
        }

        material->setTint(surfaceMaterial.tintColor);
        material->setTexture(ownTextureOf(surfaceMaterial.texture));
        material->setSurfaceMap(
            ownTextureOf(surfaceMaterial.materialMapTexture));
        material->setShadowMap(ownTextureOf(surfaceMaterial.shadowMapTexture));
        material->setLampShadows(
            ownTextureOf(surfaceMaterial.pointLightShadowAtlasTexture));
        material->setShader(ownShaderOf(surfaceMaterial.shader));

        rlDrawRenderBatchActive();
        applyRegionViewport();

        const ::Matrix wasProjection = rlGetMatrixProjection();
        const ::Matrix wasModelview = rlGetMatrixModelview();
        const bool blended = surfaceMaterial.blend == BlendMode::Alpha;

        rlSetMatrixProjection(toRaylib(camera.getProjectionMatrix()));
        rlSetMatrixModelview(toRaylib(camera.getView()));
        rlEnableDepthTest();

        if (blended)
        {
            rlDisableDepthMask();
        }

        DrawMesh(mine->getRawHandle(), material->getRawHandle(), toRaylib(modelMatrix));

        if (blended)
        {
            rlEnableDepthMask();
        }

        rlDisableDepthTest();
        rlSetMatrixModelview(wasModelview);
        rlSetMatrixProjection(wasProjection);

        material->restoreDefaults();
    }

}
