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
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>

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
        MOCK_METHOD(
            void, drawLine, (PointF from, PointF to, Color color), (override));
        MOCK_METHOD(
            void,
            drawText,
            (PointF origin,
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
            void,
            drawMesh,
            (const IMesh &mesh,
             const Mat4 &model,
             const Camera3D &camera,
             Color tint),
            (override));
        MOCK_METHOD(
            void, pushTransform, (const Mat4 &transform), (override));
        MOCK_METHOD(void, popTransform, (), (override));
        MOCK_METHOD(void, present, (), (override));
    };

}
