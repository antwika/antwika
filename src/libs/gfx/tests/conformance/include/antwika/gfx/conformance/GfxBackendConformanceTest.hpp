#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/Transform.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/fakes/FakeForeignMesh.hpp>
#include <antwika/gfx/fakes/FakeForeignTexture.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

namespace antwika::gfx::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;

    inline constexpr std::uint32_t kPollLimit = 1000;

    template <typename BackendTraits>
    class GfxBackendConformanceTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] static WindowDesc demoDesc()
        {
            return WindowDesc{
                .title = "Antwika conformance",
                .size = {.width = 640, .height = 480}};
        }

        [[nodiscard]] static WindowDesc resizableDesc()
        {
            return WindowDesc{
                .title = "Antwika conformance, resizable",
                .size = {.width = 320, .height = 240},
                .resizable = true};
        }

        [[nodiscard]] static WindowDesc fullscreenDesc()
        {
            return WindowDesc{
                .title = "Antwika conformance, fullscreen",
                .size = {.width = 480, .height = 360},
                .fullscreen = true};
        }

        [[nodiscard]] static Bitmap demoBitmap()
        {
            constexpr std::uint32_t kSide = 4;

            return Bitmap{
                .size = {.width = kSide, .height = kSide},
                .pixels = std::vector<std::uint8_t>(
                    kSide * kSide * kBytesPerPixel, 128)};
        }

        [[nodiscard]] static Rect wholeBitmap()
        {
            return Rect{
                .origin = {.x = 0, .y = 0},
                .size = demoBitmap().size};
        }

        [[nodiscard]] static MeshData demoMesh()
        {
            return MeshData{
                .vertices =
                    {Vertex3D{
                         .position = {-1.0F, -1.0F, 0.0F},
                         .texCoord = {0.0F, 0.0F},
                         .color = {.red = 255}},
                     Vertex3D{
                         .position = {1.0F, -1.0F, 0.0F},
                         .texCoord = {1.0F, 0.0F},
                         .color = {.green = 255}},
                     Vertex3D{
                         .position = {1.0F, 1.0F, 0.0F},
                         .texCoord = {1.0F, 1.0F},
                         .color = {.blue = 255}},
                     Vertex3D{
                         .position = {-1.0F, 1.0F, 0.0F},
                         .texCoord = {0.0F, 1.0F},
                         .color = {.red = 255, .green = 255}}},
                .indices = {0, 1, 2, 0, 2, 3}};
        }

        [[nodiscard]] static Camera3D demoCamera()
        {
            return Camera3D{
                Vec3{0.0F, 0.0F, 4.0F},
                Vec3{0.0F, 0.0F, 0.0F},
                Vec3{0.0F, 1.0F, 0.0F},
                Perspective{
                    .fovYRadians = 1.0F,
                    .aspectRatio = 640.0F / 480.0F}};
        }

        [[nodiscard]] static IRenderer &rendererOf(IWindow &window)
        {
            return window.renderer();
        }

        ::testing::NiceMock<MockLogger> logger;
        std::unique_ptr<IGfxBackend> backend{BackendTraits::create(logger)};
    };

    TYPED_TEST_SUITE_P(GfxBackendConformanceTest);

    TYPED_TEST_P(GfxBackendConformanceTest, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ReturnsAnOpenWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_GivesTheWindowARealId)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_NE(window->id(), kNullWindowId);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, MaxWindows_IsAtLeastOne)
    {
        EXPECT_GE(this->backend->maxWindows(), std::size_t{1});
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_GivesEachWindowItsOwnId)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());

        EXPECT_NE(first->id(), second->id());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_RefusesToExceedItsLimit)
    {
        if (this->backend->maxWindows() != 1)
        {
            GTEST_SKIP() << "backend allows more than one window";
        }

        const auto first = this->backend->createWindow(this->demoDesc());

        EXPECT_THROW(
            {
                const auto second =
                    this->backend->createWindow(this->demoDesc());
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_ReportsTheRequestedTitle)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_EQ(window->title(), "Antwika conformance");
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ReportsANonZeroSize)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, ConfiguredSize_IsExactlyWhatWasAskedFor)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_EQ(window->configuredSize(), this->demoDesc().size);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, ConfiguredSize_IsUnchangedByClosingTheWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_EQ(window->configuredSize(), this->demoDesc().size);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, ConfiguredSize_IsPerWindow)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second =
            this->backend->createWindow(this->resizableDesc());

        EXPECT_EQ(first->configuredSize(), this->demoDesc().size);
        EXPECT_EQ(second->configuredSize(), this->resizableDesc().size);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_AcceptsAResizableWindow)
    {
        const auto window =
            this->backend->createWindow(this->resizableDesc());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());

        EXPECT_EQ(window->configuredSize(), this->resizableDesc().size);
        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_AcceptsAFullscreenWindow)
    {
        const auto window =
            this->backend->createWindow(this->fullscreenDesc());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());

        EXPECT_EQ(window->configuredSize(), this->fullscreenDesc().size);
        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetFullscreen_LeavesTheWindowUsable)
    {
        const auto window =
            this->backend->createWindow(this->resizableDesc());

        window->setFullscreen(true);
        window->setFullscreen(true);
        window->setFullscreen(false);

        EXPECT_EQ(window->configuredSize(), this->resizableDesc().size);
        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
        EXPECT_TRUE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetFullscreen_IsHarmlessOnceClosed)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_NO_THROW(window->setFullscreen(true));
        EXPECT_EQ(window->configuredSize(), this->demoDesc().size);
        EXPECT_GT(window->size().width, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Size_StaysNonZeroAfterClosing)
    {
        const auto window =
            this->backend->createWindow(this->resizableDesc());

        window->close();

        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ThrowsWhenWidthIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowDesc{
                    .title = "Antwika conformance",
                    .size = {.width = 0, .height = 480}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ThrowsWhenHeightIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowDesc{
                    .title = "Antwika conformance",
                    .size = {.width = 640, .height = 0}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_ReturnsIndependentWindows)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());

        first->close();

        EXPECT_FALSE(first->isOpen());
        EXPECT_TRUE(second->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetTitle_ReplacesTheTitle)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->setTitle("Antwika renamed");

        EXPECT_EQ(window->title(), "Antwika renamed");
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Close_ClosesTheWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Close_IsIdempotent)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_NO_THROW(window->close());
        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 Renderer_AcceptsAFrameWithoutThrowing)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.clear(Color{.red = 8, .green = 8, .blue = 8});
            renderer.drawRect(
                Rect{
                    .origin = {.x = 1, .y = 2},
                    .size = {.width = 3, .height = 4}},
                Color{.red = 255});
            renderer.drawText(
                Point{.x = 8, .y = 8}, "Antwika 123", 2, Color{.green = 255});
            renderer.drawLine(
                Point{.x = 4, .y = 4},
                Point{.x = 40, .y = 22},
                Color{.blue = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Renderer_AcceptsATranslucentFrame)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.clear(Color{.red = 200});
            renderer.drawRect(
                Rect{
                    .origin = {.x = 4, .y = 4},
                    .size = {.width = 32, .height = 32}},
                Color{.blue = 255, .alpha = 48});
            renderer.drawLine(
                Point{.x = 0, .y = 0},
                Point{.x = 32, .y = 32},
                Color{.green = 255, .alpha = 128});
            renderer.drawText(
                Point{.x = 2, .y = 2}, "faint", 1,
                Color{.red = 255, .alpha = 1});
            renderer.drawRect(
                Rect{
                    .origin = {.x = 8, .y = 8},
                    .size = {.width = 8, .height = 8}},
                Color{.red = 255, .alpha = 0});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawLine_AcceptsAwkwardLines)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.drawLine(
                Point{.x = 9, .y = 9},
                Point{.x = 9, .y = 9},
                Color{.red = 255});
            renderer.drawLine(
                Point{.x = 60, .y = 30},
                Point{.x = 10, .y = 5},
                Color{.red = 255});
            renderer.drawLine(
                Point{.x = -80, .y = -40},
                Point{.x = -10, .y = -10},
                Color{.red = 255});
            renderer.drawLine(
                Point{.x = -20, .y = 240},
                Point{.x = 900, .y = 260},
                Color{.red = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawText_AcceptsAwkwardText)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.drawText(Point{}, "", 2, Color{.red = 255});
            renderer.drawText(Point{}, "As", 0, Color{.red = 255});
            renderer.drawText(Point{}, "\n\t\x7f", 2, Color{.red = 255});
            renderer.drawText(
                Point{.x = -50, .y = -50}, "off canvas", 3,
                Color{.red = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateTexture_ReportsTheBitmapSize)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        const auto texture =
            window->renderer().createTexture(this->demoBitmap());

        ASSERT_NE(texture, nullptr);

        EXPECT_EQ(texture->size(), this->demoBitmap().size);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, CreateTexture_ThrowsOnAnIncompleteBitmap)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_THROW(
            { const auto texture = renderer.createTexture(Bitmap{}); },
            GfxError);

        EXPECT_THROW(
            {
                const auto texture = renderer.createTexture(
                    Bitmap{
                        .size = {.width = 4, .height = 4},
                        .pixels = {}});
            },
            GfxError);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawTexture_AcceptsAFrameWithoutThrowing)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();
        const auto texture = renderer.createTexture(this->demoBitmap());
        const auto whole = this->wholeBitmap();

        EXPECT_NO_THROW({
            renderer.clear(Color{});
            renderer.drawTexture(
                *texture, whole,
                Rect{
                    .origin = {.x = 8, .y = 8},
                    .size = {.width = 4, .height = 4}},
                Color{.red = 255, .green = 255, .blue = 255});
            renderer.drawTexture(
                *texture,
                Rect{
                    .origin = {.x = 1, .y = 1},
                    .size = {.width = 2, .height = 2}},
                Rect{
                    .origin = {.x = 40, .y = 40},
                    .size = {.width = 64, .height = 64}},
                Color{.red = 255, .green = 80, .blue = 80, .alpha = 128});
            renderer.drawTexture(
                *texture, whole,
                Rect{
                    .origin = {.x = -20, .y = -20},
                    .size = {.width = 32, .height = 32}},
                Color{.red = 255, .green = 255, .blue = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawTexture_AcceptsAnUndrawableBlit)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();
        const auto texture = renderer.createTexture(this->demoBitmap());
        const auto whole = this->wholeBitmap();
        const Color white{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            renderer.drawTexture(*texture, Rect{}, whole, white);
            renderer.drawTexture(*texture, whole, Rect{}, white);
            renderer.drawTexture(
                *texture,
                Rect{
                    .origin = {.x = -1, .y = -1},
                    .size = {.width = 4, .height = 4}},
                whole, white);
            renderer.drawTexture(
                *texture,
                Rect{
                    .origin = {.x = 2, .y = 2},
                    .size = {.width = 99, .height = 99}},
                whole, white);
            renderer.present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest,
        DrawTexture_AcceptsATextureFromAnotherRenderer)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());
        const auto texture =
            first->renderer().createTexture(this->demoBitmap());

        EXPECT_NO_THROW({
            second->renderer().drawTexture(
                *texture, this->wholeBitmap(), this->wholeBitmap(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawTexture_AcceptsATextureOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        const fakes::FakeForeignTexture foreign;
        const auto whole = this->wholeBitmap();

        EXPECT_NO_THROW({
            window->renderer().drawTexture(
                foreign, whole, whole,
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Texture_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->demoDesc());
        auto texture = window->renderer().createTexture(this->demoBitmap());

        window->close();

        EXPECT_NO_THROW(window->renderer().drawTexture(
            *texture, this->wholeBitmap(), this->wholeBitmap(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(texture.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateMesh_ReportsTheGeometrysCounts)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = this->rendererOf(*window);


        const auto mesh = renderer.createMesh(this->demoMesh());

        ASSERT_NE(mesh, nullptr);

        EXPECT_EQ(mesh->vertexCount(), this->demoMesh().vertices.size());
        EXPECT_EQ(mesh->triangleCount(), this->demoMesh().triangleCount());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateMesh_ThrowsOnIncompleteData)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = this->rendererOf(*window);


        EXPECT_THROW(
            { const auto mesh = renderer.createMesh(MeshData{}); },
            GfxError);

        EXPECT_THROW(
            {
                const auto mesh = renderer.createMesh(
                    MeshData{
                        .vertices = {Vertex3D{}},
                        .indices = {0, 1, 2}});
            },
            GfxError);

        EXPECT_THROW(
            {
                const auto mesh = renderer.createMesh(
                    MeshData{
                        .vertices = {Vertex3D{}, Vertex3D{}, Vertex3D{}},
                        .indices = {0, 1}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_AcceptsAFrameWithoutThrowing)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &flat = window->renderer();
        auto &renderer = this->rendererOf(*window);


        const auto mesh = renderer.createMesh(this->demoMesh());
        const auto camera = this->demoCamera();
        const Color white{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            flat.clear(Color{.red = 8, .green = 8, .blue = 16});
            renderer.drawMesh(*mesh, identityMatrix(), camera, white);
            renderer.drawMesh(
                *mesh,
                Transform{
                    .translation = {0.5F, 0.0F, -1.0F},
                    .rotationRadians = {0.3F, 0.7F, 0.0F},
                    .scale = {0.5F, 0.5F, 0.5F}}
                    .matrix(),
                camera,
                Color{.red = 255, .green = 128, .blue = 0, .alpha = 128});
            flat.drawRect(
                Rect{
                    .origin = {.x = 4, .y = 4},
                    .size = {.width = 16, .height = 16}},
                Color{.green = 255});
            flat.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawMesh_AcceptsAnAwkwardCamera)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = this->rendererOf(*window);


        const auto mesh = renderer.createMesh(this->demoMesh());
        const Color white{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            renderer.drawMesh(
                *mesh,
                identityMatrix(),
                Camera3D{
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 1.0F, 0.0F},
                    Perspective{}},
                white);
            renderer.drawMesh(
                *mesh,
                identityMatrix(),
                Camera3D{
                    Vec3{2.0F, 2.0F, 2.0F},
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 1.0F, 0.0F},
                    Orthographic{.halfWidth = 3.0F, .halfHeight = 2.0F}},
                white);
            window->renderer().present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawMesh_AcceptsAMeshFromAnotherRenderer)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());
        auto &mine = this->rendererOf(*first);
        auto &theirs = this->rendererOf(*second);


        const auto mesh = mine.createMesh(this->demoMesh());

        EXPECT_NO_THROW({
            theirs.drawMesh(
                *mesh,
                identityMatrix(),
                this->demoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawMesh_AcceptsAMeshOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = this->rendererOf(*window);


        const fakes::FakeForeignMesh foreign;

        EXPECT_NO_THROW({
            renderer.drawMesh(
                foreign,
                identityMatrix(),
                this->demoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Mesh_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = this->rendererOf(*window);


        auto mesh = renderer.createMesh(this->demoMesh());

        window->close();

        EXPECT_NO_THROW(renderer.drawMesh(
            *mesh,
            identityMatrix(),
            this->demoCamera(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(mesh.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, PollEvent_DrainsToAnEmptyQueue)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        std::uint32_t polls = 0;

        while (this->backend->pollEvent())
        {
            ++polls;

            ASSERT_LT(polls, kPollLimit);
        }

        SUCCEED();
    }

    TYPED_TEST_P(GfxBackendConformanceTest, PollEvent_DrainsAfterAFrameIsDrawn)
    {
        const auto window = this->backend->createWindow(WindowDesc{
            .title = "Antwika conformance",
            .size = {.width = 640, .height = 480},
            .resizable = true});

        auto &renderer = window->renderer();

        for (std::uint32_t frame = 0; frame < 3; ++frame)
        {
            renderer.clear(Color{.red = 8, .green = 8, .blue = 8});
            renderer.present();

            std::uint32_t polls = 0;

            while (this->backend->pollEvent())
            {
                ++polls;

                ASSERT_LT(polls, kPollLimit);
            }
        }

        SUCCEED();
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Window_MayOutliveItsBackend)
    {
        auto window = this->backend->createWindow(this->demoDesc());

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
    }

    REGISTER_TYPED_TEST_SUITE_P(
        GfxBackendConformanceTest,
        Name_IsNotEmpty,
        MaxWindows_IsAtLeastOne,
        CreateWindow_ReturnsAnOpenWindow,
        CreateWindow_GivesTheWindowARealId,
        CreateWindow_GivesEachWindowItsOwnId,
        CreateWindow_RefusesToExceedItsLimit,
        CreateWindow_ReportsTheRequestedTitle,
        CreateWindow_ReportsANonZeroSize,
        ConfiguredSize_IsExactlyWhatWasAskedFor,
        ConfiguredSize_IsUnchangedByClosingTheWindow,
        ConfiguredSize_IsPerWindow,
        CreateWindow_AcceptsAResizableWindow,
        CreateWindow_AcceptsAFullscreenWindow,
        SetFullscreen_LeavesTheWindowUsable,
        SetFullscreen_IsHarmlessOnceClosed,
        Size_StaysNonZeroAfterClosing,
        CreateWindow_ThrowsWhenWidthIsZero,
        CreateWindow_ThrowsWhenHeightIsZero,
        CreateWindow_ReturnsIndependentWindows,
        SetTitle_ReplacesTheTitle,
        Close_ClosesTheWindow,
        Close_IsIdempotent,
        Renderer_AcceptsAFrameWithoutThrowing,
        Renderer_AcceptsATranslucentFrame,
        DrawLine_AcceptsAwkwardLines,
        DrawText_AcceptsAwkwardText,
        CreateTexture_ReportsTheBitmapSize,
        CreateTexture_ThrowsOnAnIncompleteBitmap,
        DrawTexture_AcceptsAFrameWithoutThrowing,
        DrawTexture_AcceptsAnUndrawableBlit,
        DrawTexture_AcceptsATextureFromAnotherRenderer,
        DrawTexture_AcceptsATextureOfAnotherKind,
        Texture_MayOutliveItsWindow,
        CreateMesh_ReportsTheGeometrysCounts,
        CreateMesh_ThrowsOnIncompleteData,
        DrawMesh_AcceptsAFrameWithoutThrowing,
        DrawMesh_AcceptsAnAwkwardCamera,
        DrawMesh_AcceptsAMeshFromAnotherRenderer,
        DrawMesh_AcceptsAMeshOfAnotherKind,
        Mesh_MayOutliveItsWindow,
        PollEvent_DrainsToAnEmptyQueue,
        PollEvent_DrainsAfterAFrameIsDrawn,
        Window_MayOutliveItsBackend);

}
