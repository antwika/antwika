#pragma once

#include <gmock/gmock.h>

#include <cstdint>
#include <memory>
#include <string_view>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ShaderSource.hpp>

namespace antwika::gfx::mocks
{

    using antwika::gfx::IRenderer;

    using antwika::gfx::RectF;

    using antwika::gfx::PointF;

    class MockRenderer : public IRenderer
    {
    public:
        MOCK_METHOD(void, clear, (Color color), (override));
        MOCK_METHOD(void, drawRect, (RectF rect, Color color), (override));
        MOCK_METHOD(void, beginClip, (RectF area), (override));
        MOCK_METHOD(void, endClip, (), (override));
        MOCK_METHOD(
            void, drawLine, (PointF fromPoint, PointF toPoint, Color color),
            (override));
        MOCK_METHOD(
            void,
            drawText,
            (PointF originPoint,
             std::string_view text,
             std::uint32_t scale,
             Color color),
            (override));
        MOCK_METHOD(
            std::unique_ptr<ITexture>,
            createTexture,
            (const Bitmap &bitmap),
            (override));
        MOCK_METHOD(
            void,
            updateTexture,
            (ITexture & texture, const Bitmap &bitmap),
            (override));

        MOCK_METHOD(
            void,
            updateMesh,
            (antwika::gfx::IMesh & mesh,
                const antwika::gfx::MeshData &data),
            (override));
        MOCK_METHOD(
            void,
            drawTexture,
            (const ITexture &texture,
             RectF source,
             RectF destination,
             Color tint),
            (override));
        MOCK_METHOD(
            std::unique_ptr<IMesh>,
            createMesh,
            (const MeshData &mesh),
            (override));
        MOCK_METHOD(
            std::unique_ptr<IShader>,
            createShader,
            (const ShaderSource &source),
            (override));

        MOCK_METHOD(
            void,
            setShaderNumber,
            (const IShader &shader,
             std::string_view name,
             float value),
            (override));
        MOCK_METHOD(
            void,
            setShaderVector,
            (const IShader &shader,
             std::string_view name,
             Vec3 value),
            (override));
        MOCK_METHOD(
            std::unique_ptr<IRenderTarget>,
            createRenderTarget,
            (const RenderTargetSpec &spec),
            (override));
        MOCK_METHOD(
            void, beginTarget, (IRenderTarget & target), (override));

        MOCK_METHOD(
            void,
            beginTargetRegion,
            (IRenderTarget & target, Rect region),
            (override));
        MOCK_METHOD(void, endTarget, (), (override));
        MOCK_METHOD(
            void,
            setShaderMatrix,
            (const IShader &shader,
             std::string_view name,
             const Mat4 &value),
            (override));
        MOCK_METHOD(
            void,
            setShaderColor,
            (const IShader &shader,
             std::string_view name,
             Color value),
            (override));

        using IRenderer::drawMesh;

        MOCK_METHOD(
            void,
            drawMesh,
            (const IMesh &mesh,
             const Mat4 &model,
             const Camera3D &camera,
             const MeshMaterial &material),
            (override));
        MOCK_METHOD(
            void, pushTransform, (const Mat4 &transform), (override));
        MOCK_METHOD(void, popTransform, (), (override));
        MOCK_METHOD(Bitmap, readPixels, (), (override));
        MOCK_METHOD(void, present, (), (override));
    };

}
