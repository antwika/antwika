#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/fakes/FakeBareTarget.hpp>
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
using antwika::gfx::IMesh;
using antwika::gfx::Mat4;
using antwika::gfx::MeshData;
using antwika::gfx::MeshMaterial;
using antwika::gfx::Orthographic;
using antwika::gfx::Perspective;
using antwika::gfx::Vec3;
using antwika::gfx::ShaderSource;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockShader;
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
    constexpr Size kCanvasSize{.width = 100, .height = 50};

    constexpr Size kWindowSize{.width = 300, .height = 100};

    constexpr Color kInkColor{.red = 1, .green = 2, .blue = 3};

    constexpr Color kWhiteColor{.red = 255, .green = 255, .blue = 255};

    constexpr float kSubPixel = 0.01F;

    Camera3D canvasCamera()
    {
        const auto halfWidth =
            static_cast<float>(kCanvasSize.width) / 2.0F;
        const auto halfHeight =
            static_cast<float>(kCanvasSize.height) / 2.0F;

        return Camera3D{
            Vec3{halfWidth, -halfHeight, 1.0F},
            Vec3{halfWidth, -halfHeight, 0.0F},
            Vec3{0.0F, 1.0F, 0.0F},
            Orthographic{
                .halfWidth = halfWidth, .halfHeight = halfHeight}};
    }

    struct CanvasSample final
    {
        PointF canvasPoint;
        PointF windowPoint;
    };

    PointF windowOf(
        const Camera3D &camera,
        const PointF canvasPoint,
        const Size windowSize = kWindowSize)
    {
        const auto clip =
            camera.viewProjection()
            * antwika::gfx::Vec4{canvasPoint.x, -canvasPoint.y, 0.0F, 1.0F};

        return PointF{
            ((clip.x / clip.w) + 1.0F) * 0.5F
                * static_cast<float>(windowSize.width),
            (1.0F - (clip.y / clip.w)) * 0.5F
                * static_cast<float>(windowSize.height)};
    }
}

TEST(ViewportRendererTest, Viewport_IsTheOneViewportForBothSizes)
{
    NiceMock<MockRenderer> innerRenderer;
    const ViewportRenderer viewportRenderer(
        innerRenderer,
        kWindowSize,
        kCanvasSize);

    EXPECT_EQ(
        viewportRenderer.viewport(),
        (Viewport{
            .offsetPoint = {.x = 50, .y = 0},
            .numerator = 2,
            .denominator = 1}));
}

TEST(ViewportRendererTest, Clear_FillsTheWholeDrawableArea)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, clear(kInkColor));

    viewportRenderer.clear(kInkColor);
}

TEST(ViewportRendererTest, DrawRect_ScalesAndOffsetsTheRectangle)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(
        innerRenderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 70, .y = 20},
                .size = {.width = 20, .height = 8}}},
            kInkColor));

    viewportRenderer.drawRect(
        Rect{
            .originPoint = {.x = 10, .y = 10},
            .size = {.width = 10, .height = 4}},
        kInkColor);
}

TEST(ViewportRendererTest, BeginClip_ScalesAndOffsetsTheArea)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(
        innerRenderer,
        beginClip(
            RectF{Rect{
                .originPoint = {.x = 70, .y = 20},
                .size = {.width = 20, .height = 8}}}));

    viewportRenderer.beginClip(
        RectF{Rect{
            .originPoint = {.x = 10, .y = 10},
            .size = {.width = 10, .height = 4}}});
}

TEST(ViewportRendererTest, BeginClip_IsRawWhileATargetIsBound)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    antwika::gfx::fakes::FakeBareTarget target;

    const RectF areaRect{Rect{
        .originPoint = {.x = 10, .y = 10},
        .size = {.width = 10, .height = 4}}};

    EXPECT_CALL(innerRenderer, beginClip(areaRect));

    viewportRenderer.beginTarget(target);
    viewportRenderer.beginClip(areaRect);
    viewportRenderer.endTarget();
}

TEST(ViewportRendererTest, EndClip_ReachesTheWrappedRenderer)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, endClip());

    viewportRenderer.endClip();
}

TEST(ViewportRendererTest, DrawLine_ScalesAndOffsetsBothEnds)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(
        innerRenderer,
        drawLine(PointF{50.0F, 0.0F}, PointF{250.0F, 100.0F}, kInkColor));

    viewportRenderer.drawLine(
        Point{.x = 0, .y = 0},
        Point{.x = 100, .y = 50},
        kInkColor);
}

TEST(ViewportRendererTest, DrawText_ScalesTheOriginAndTheGlyphScale)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(
        innerRenderer, drawText(
            PointF{70.0F, 20.0F},
            "hi",
            std::uint32_t{4},
            kInkColor));

    viewportRenderer.drawText(Point{.x = 10, .y = 10}, "hi", 2, kInkColor);
}

TEST(ViewportRendererTest, DrawText_AnchorsEachGlyphOnANonWholeScale)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(
        innerRenderer, Size{.width = 150, .height = 75}, kCanvasSize);

    EXPECT_CALL(
        innerRenderer, drawText(
            PointF{15.0F, 15.0F},
            "h",
            std::uint32_t{1},
            kInkColor));
    EXPECT_CALL(
        innerRenderer, drawText(
            PointF{24.0F, 15.0F},
            "i",
            std::uint32_t{1},
            kInkColor));

    viewportRenderer.drawText(Point{.x = 10, .y = 10}, "hi", 1, kInkColor);
}

TEST(ViewportRendererTest, DrawTexture_ScalesTheDestinationAndNotTheSource)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockTexture> texture;

    const Rect sourceRect{
        .originPoint = {.x = 4, .y = 4}, .size = {.width = 8, .height = 8}};

    EXPECT_CALL(
        innerRenderer,
        drawTexture(
            _,
            RectF{sourceRect},
            RectF{Rect{
                .originPoint = {.x = 50, .y = 0},
                .size = {.width = 200, .height = 100}}},
            kInkColor));

    viewportRenderer.drawTexture(
        texture,
        sourceRect,
        Rect{
            .originPoint = {.x = 0, .y = 0},
            .size = {.width = 100, .height = 50}},
        kInkColor);
}

TEST(ViewportRendererTest, CreateTexture_ComesFromTheWrappedRenderer)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, createTexture(_))
        .WillOnce(
            []([[maybe_unused]] const Bitmap &bitmap)
            { return std::unique_ptr<ITexture>{}; });

    EXPECT_EQ(viewportRenderer.createTexture(Bitmap{}), nullptr);
}

TEST(ViewportRendererTest, CreateMesh_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, createMesh(_));

    (void)viewportRenderer.createMesh(MeshData{});
}

TEST(ViewportRendererTest, DrawMesh_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockMesh> mesh;
    const Camera3D camera;
    const Color tintColor{.red = 255, .green = 255, .blue = 255};

    EXPECT_CALL(
        innerRenderer, drawMesh(
            Ref(mesh),
            _,
            _,
            MeshMaterial{.tintColor = tintColor}));

    viewportRenderer.drawMesh(mesh, identityMatrix(), camera, tintColor);
}

TEST(ViewportRendererTest, DrawMesh_CarriesTheWholeMaterialThrough)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockMesh> mesh;
    const NiceMock<MockTexture> texture;
    const NiceMock<MockShader> shader;
    const MeshMaterial material{
        .texture = &texture,
        .shader = &shader,
        .tintColor = {.red = 32, .green = 64, .blue = 96}};

    EXPECT_CALL(innerRenderer, drawMesh(Ref(mesh), _, _, material));

    viewportRenderer.drawMesh(mesh, identityMatrix(), Camera3D{}, material);
}

TEST(ViewportRendererTest, DrawMesh_LandsAWorldPointWhereTheCanvasPixelIs)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockMesh> mesh;

    Camera3D placedCamera;

    EXPECT_CALL(innerRenderer, drawMesh(Ref(mesh), _, _, _))
        .WillOnce(
            [&placedCamera](
                const IMesh &,
                const Mat4 &,
                const Camera3D &camera,
                const MeshMaterial &) { placedCamera = camera; });

    viewportRenderer.drawMesh(
        mesh,
        identityMatrix(),
        canvasCamera(),
        kWhiteColor);

    for (const auto sample : {
             CanvasSample{{0.0F, 0.0F}, {50.0F, 0.0F}},
             CanvasSample{{100.0F, 50.0F}, {250.0F, 100.0F}},
             CanvasSample{{50.0F, 25.0F}, {150.0F, 50.0F}},
             CanvasSample{{-25.0F, 10.0F}, {0.0F, 20.0F}}})
    {
        const auto got = windowOf(placedCamera, sample.canvasPoint);

        EXPECT_NEAR(sample.windowPoint.x, got.x, kSubPixel);
        EXPECT_NEAR(sample.windowPoint.y, got.y, kSubPixel);
    }
}

TEST(ViewportRendererTest, DrawMesh_KeepsTheCanvasCentredWhenLetterboxed)
{
    constexpr Size kTallWindowSize{.width = 300, .height = 300};

    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(
        innerRenderer,
        kTallWindowSize,
        kCanvasSize);
    const NiceMock<MockMesh> mesh;

    Camera3D placedCamera;

    EXPECT_CALL(innerRenderer, drawMesh(Ref(mesh), _, _, _))
        .WillOnce(
            [&placedCamera](
                const IMesh &,
                const Mat4 &,
                const Camera3D &camera,
                const MeshMaterial &) { placedCamera = camera; });

    viewportRenderer.drawMesh(
        mesh,
        identityMatrix(),
        canvasCamera(),
        kWhiteColor);

    for (const auto sample : {
             CanvasSample{{0.0F, 0.0F}, {0.0F, 75.0F}},
             CanvasSample{{100.0F, 50.0F}, {300.0F, 225.0F}},
             CanvasSample{{50.0F, 25.0F}, {150.0F, 150.0F}}})
    {
        const auto got = windowOf(
            placedCamera,
            sample.canvasPoint,
            kTallWindowSize);

        EXPECT_NEAR(sample.windowPoint.x, got.x, kSubPixel);
        EXPECT_NEAR(sample.windowPoint.y, got.y, kSubPixel);
    }
}

TEST(ViewportRendererTest, DrawMesh_TracksAResize)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockMesh> mesh;

    constexpr Size kGrownSize{.width = 400, .height = 200};

    viewportRenderer.resize(kGrownSize);

    Camera3D placedCamera;

    EXPECT_CALL(innerRenderer, drawMesh(Ref(mesh), _, _, _))
        .WillOnce(
            [&placedCamera](
                const IMesh &,
                const Mat4 &,
                const Camera3D &camera,
                const MeshMaterial &) { placedCamera = camera; });

    viewportRenderer.drawMesh(
        mesh,
        identityMatrix(),
        canvasCamera(),
        kWhiteColor);

    const auto got = windowOf(placedCamera, {0.0F, 0.0F}, kGrownSize);

    EXPECT_NEAR(0.0F, got.x, kSubPixel);
    EXPECT_NEAR(0.0F, got.y, kSubPixel);
}

TEST(ViewportRendererTest, DrawMesh_LeavesAPerspectiveCameraAlone)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockMesh> mesh;

    const Camera3D camera{
        Vec3{0.0F, 0.0F, 4.0F},
        Vec3{0.0F, 0.0F, 0.0F},
        Vec3{0.0F, 1.0F, 0.0F},
        Perspective{.fovYRadians = 1.0F, .aspectRatio = 2.0F}};

    Camera3D placedCamera;

    EXPECT_CALL(innerRenderer, drawMesh(Ref(mesh), _, _, _))
        .WillOnce(
            [&placedCamera](
                const IMesh &,
                const Mat4 &,
                const Camera3D &seenCamera,
                const MeshMaterial &) { placedCamera = seenCamera; });

    viewportRenderer.drawMesh(mesh, identityMatrix(), camera, kWhiteColor);

    EXPECT_EQ(camera.position(), placedCamera.position());
    EXPECT_EQ(camera.projection(), placedCamera.projection());
}

TEST(ViewportRendererTest, CreateShader_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const ShaderSource source{.vertex = "vs", .fragment = "fs"};

    EXPECT_CALL(innerRenderer, createShader(source));

    (void)viewportRenderer.createShader(source);
}

TEST(ViewportRendererTest, SetShader_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    const NiceMock<MockShader> shader;

    EXPECT_CALL(innerRenderer, setShaderNumber(Ref(shader), "reach", 4.0F));
    EXPECT_CALL(
        innerRenderer, setShaderVector(
            Ref(shader),
            "at",
            Vec3{1.0F, 2.0F, 3.0F}));
    EXPECT_CALL(
        innerRenderer,
        setShaderColor(Ref(shader), "glow", kWhiteColor));

    viewportRenderer.setShaderNumber(shader, "reach", 4.0F);
    viewportRenderer.setShaderVector(shader, "at", Vec3{1.0F, 2.0F, 3.0F});
    viewportRenderer.setShaderColor(shader, "glow", kWhiteColor);
}

TEST(ViewportRendererTest, ReadPixels_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, readPixels());

    (void)viewportRenderer.readPixels();
}

TEST(ViewportRendererTest, Present_ReachesTheWrappedRenderer)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, present());

    viewportRenderer.present();
}

TEST(ViewportRendererTest, FillLetterbox_PaintsThePillarboxes)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(
        innerRenderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 0, .y = 0},
                .size = {.width = 50, .height = 100}}},
            kInkColor));
    EXPECT_CALL(
        innerRenderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 250, .y = 0},
                .size = {.width = 50, .height = 100}}},
            kInkColor));

    viewportRenderer.fillLetterbox(kInkColor);
}

TEST(ViewportRendererTest, FillLetterbox_PaintsTheLetterboxes)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(
        innerRenderer,
        Size{.width = 100, .height = 80},
        kCanvasSize);

    EXPECT_CALL(
        innerRenderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 0, .y = 0},
                .size = {.width = 100, .height = 15}}},
            kInkColor));
    EXPECT_CALL(
        innerRenderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 0, .y = 65},
                .size = {.width = 100, .height = 15}}},
            kInkColor));

    viewportRenderer.fillLetterbox(kInkColor);
}

TEST(ViewportRendererTest, FillLetterbox_DrawsNothingWhenTheCanvasFits)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kCanvasSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, drawRect(_, _)).Times(0);

    viewportRenderer.fillLetterbox(kInkColor);
}

TEST(ViewportRendererTest, EveryCall_IsUntouchedWhenTheSizesAgree)
{
    MockRenderer innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kCanvasSize, kCanvasSize);

    const Rect rect{
        .originPoint = {.x = 3, .y = 4}, .size = {.width = 5, .height = 6}};

    EXPECT_CALL(innerRenderer, drawRect(RectF{rect}, kInkColor));
    EXPECT_CALL(
        innerRenderer, drawText(
            PointF{3.0F, 4.0F},
            "x",
            std::uint32_t{2},
            kInkColor));

    viewportRenderer.drawRect(rect, kInkColor);
    viewportRenderer.drawText(Point{.x = 3, .y = 4}, "x", 2, kInkColor);
}

TEST(ViewportRendererTest, PushTransform_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, pushTransform(_));

    viewportRenderer.pushTransform(identityMatrix());
}

TEST(ViewportRendererTest, PopTransform_ReachesTheWrappedRenderer)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);

    EXPECT_CALL(innerRenderer, popTransform());

    viewportRenderer.popTransform();
}

TEST(ViewportRendererTest, Resize_RebuildsTheTransformForTheNewSize)
{
    NiceMock<MockRenderer> innerRenderer;
    ViewportRenderer renderer(
        innerRenderer, Size{.width = 200, .height = 100}, kCanvasSize);
    const auto beforeViewport = renderer.viewport();

    renderer.resize(Size{.width = 400, .height = 400});

    const auto afterViewport = renderer.viewport();

    EXPECT_NE(beforeViewport, afterViewport);
    EXPECT_EQ(
        afterViewport,
        antwika::gfx::viewportFor(
            Size{.width = 400, .height = 400}, kCanvasSize));
}
