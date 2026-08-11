#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/PointF.hpp>

#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"
#include "antwika/gfx/Viewport.hpp"
#include "antwika/gfx/ViewportRenderer.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::Camera3D;
using antwika::gfx::identityMatrix;
using antwika::gfx::MeshData;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::ITexture;
using antwika::gfx::Point;
using antwika::gfx::PointF;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::Viewport;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Ref;
using ::testing::Return;

namespace
{
    constexpr Size kCanvas{.width = 100, .height = 50};

    constexpr Size kWindow{.width = 300, .height = 100};

    constexpr Color kInk{.red = 1, .green = 2, .blue = 3};
}

TEST(ViewportRendererTest, Viewport_IsTheOneViewportForBothSizes)
{
    NiceMock<MockRenderer> inner;
    const ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_EQ(
        view.viewport(),
        (Viewport{
            .offset = {.x = 50, .y = 0},
            .numerator = 2,
            .denominator = 1}));
}

TEST(ViewportRendererTest, Clear_FillsTheWholeDrawableArea)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, clear(kInk));

    view.clear(kInk);
}

TEST(ViewportRendererTest, DrawRect_ScalesAndOffsetsTheRectangle)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            RectF{Rect{
                .origin = {.x = 70, .y = 20},
                .size = {.width = 20, .height = 8}}},
            kInk));

    view.drawRect(
        Rect{
            .origin = {.x = 10, .y = 10},
            .size = {.width = 10, .height = 4}},
        kInk);
}

TEST(ViewportRendererTest, DrawLine_ScalesAndOffsetsBothEnds)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawLine(PointF{50.0F, 0.0F}, PointF{250.0F, 100.0F}, kInk));

    view.drawLine(Point{.x = 0, .y = 0}, Point{.x = 100, .y = 50}, kInk);
}

TEST(ViewportRendererTest, DrawText_ScalesTheOriginAndTheGlyphScale)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner, drawText(PointF{70.0F, 20.0F}, "hi", std::uint32_t{4}, kInk));

    view.drawText(Point{.x = 10, .y = 10}, "hi", 2, kInk);
}

TEST(ViewportRendererTest, DrawText_AnchorsEachGlyphOnANonWholeScale)
{
    MockRenderer inner;
    ViewportRenderer view(
        inner, Size{.width = 150, .height = 75}, kCanvas);

    EXPECT_CALL(
        inner, drawText(PointF{15.0F, 15.0F}, "h", std::uint32_t{1}, kInk));
    EXPECT_CALL(
        inner, drawText(PointF{24.0F, 15.0F}, "i", std::uint32_t{1}, kInk));

    view.drawText(Point{.x = 10, .y = 10}, "hi", 1, kInk);
}

TEST(ViewportRendererTest, DrawTexture_ScalesTheDestinationAndNotTheSource)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);
    const NiceMock<MockTexture> texture;

    const Rect source{
        .origin = {.x = 4, .y = 4}, .size = {.width = 8, .height = 8}};

    EXPECT_CALL(
        inner,
        drawTexture(
            _,
            RectF{source},
            RectF{Rect{
                .origin = {.x = 50, .y = 0},
                .size = {.width = 200, .height = 100}}},
            kInk));

    view.drawTexture(
        texture,
        source,
        Rect{
            .origin = {.x = 0, .y = 0},
            .size = {.width = 100, .height = 50}},
        kInk);
}

TEST(ViewportRendererTest, CreateTexture_ComesFromTheWrappedRenderer)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, createTexture(_))
        .WillOnce(
            []([[maybe_unused]] const Bitmap &bitmap)
            { return std::unique_ptr<ITexture>{}; });

    EXPECT_EQ(view.createTexture(Bitmap{}), nullptr);
}

TEST(ViewportRendererTest, CreateMesh_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, createMesh(_));

    (void)view.createMesh(MeshData{});
}

TEST(ViewportRendererTest, DrawMesh_ReachesTheWrappedRendererUnscaled)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kWindow, kCanvas);
    const NiceMock<MockMesh> mesh;
    const Camera3D camera;
    const Color tint{.red = 255, .green = 255, .blue = 255};

    EXPECT_CALL(inner, drawMesh(Ref(mesh), _, _, tint));

    view.drawMesh(mesh, identityMatrix(), camera, tint);
}

TEST(ViewportRendererTest, Present_ReachesTheWrappedRenderer)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, present());

    view.present();
}

TEST(ViewportRendererTest, FillSurround_PaintsThePillarboxes)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 50, .height = 100}}},
            kInk));
    EXPECT_CALL(
        inner,
        drawRect(
            RectF{Rect{
                .origin = {.x = 250, .y = 0},
                .size = {.width = 50, .height = 100}}},
            kInk));

    view.fillSurround(kInk);
}

TEST(ViewportRendererTest, FillSurround_PaintsTheLetterboxes)
{
    MockRenderer inner;
    ViewportRenderer view(inner, Size{.width = 100, .height = 80}, kCanvas);

    EXPECT_CALL(
        inner,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 0},
                .size = {.width = 100, .height = 15}}},
            kInk));
    EXPECT_CALL(
        inner,
        drawRect(
            RectF{Rect{
                .origin = {.x = 0, .y = 65},
                .size = {.width = 100, .height = 15}}},
            kInk));

    view.fillSurround(kInk);
}

TEST(ViewportRendererTest, FillSurround_DrawsNothingWhenTheCanvasFits)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    EXPECT_CALL(inner, drawRect(_, _)).Times(0);

    view.fillSurround(kInk);
}

TEST(ViewportRendererTest, EveryCall_IsUntouchedWhenTheSizesAgree)
{
    MockRenderer inner;
    ViewportRenderer view(inner, kCanvas, kCanvas);

    const Rect rect{
        .origin = {.x = 3, .y = 4}, .size = {.width = 5, .height = 6}};

    EXPECT_CALL(inner, drawRect(RectF{rect}, kInk));
    EXPECT_CALL(
        inner, drawText(PointF{3.0F, 4.0F}, "x", std::uint32_t{2}, kInk));

    view.drawRect(rect, kInk);
    view.drawText(Point{.x = 3, .y = 4}, "x", 2, kInk);
}

TEST(ViewportRendererTest, PushTransform_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, pushTransform(_));

    view.pushTransform(identityMatrix());
}

TEST(ViewportRendererTest, PopTransform_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer view(inner, kWindow, kCanvas);

    EXPECT_CALL(inner, popTransform());

    view.popTransform();
}

TEST(ViewportRendererTest, Resize_RebuildsTheTransformForTheNewSize)
{
    NiceMock<MockRenderer> inner;
    ViewportRenderer renderer(
        inner, Size{.width = 200, .height = 100}, kCanvas);
    const auto before = renderer.viewport();

    renderer.resize(Size{.width = 400, .height = 400});

    const auto after = renderer.viewport();

    EXPECT_NE(before, after);
    EXPECT_EQ(
        after,
        antwika::gfx::viewportFor(
            Size{.width = 400, .height = 400}, kCanvas));
}
