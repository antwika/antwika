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
#include <antwika/gfx/IRenderer3D.hpp>
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
#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

namespace antwika::gfx::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;

    /**
     * @brief How many polls a drained event queue is allowed to take
     * before the backend is declared to be looping forever.
     */
    inline constexpr std::uint32_t kPollLimit = 1000;

    /**
     * @brief A texture no backend made, and none may reach inside.
     *
     * ITexture is opaque, so a renderer handed one has to ask whether
     * it is even its own before reaching for a native handle.
     * The two-window tests ask that question of a texture from a
     * sibling renderer, and skip on a backend that allows one window;
     * this asks it of an implementation no backend has ever seen, which
     * every backend can be asked.
     */
    class ForeignTexture final : public ITexture
    {
    public:
        /**
         * @brief Report a size, as any texture must.
         * @return A plausible one; nothing should be asking.
         */
        [[nodiscard]] Size size() const override
        {
            return Size{.width = 4, .height = 4};
        }
    };

    /**
     * @brief A mesh no backend made, and none may reach inside.
     *
     * IMesh's answer to ForeignTexture, for the same question.
     */
    class ForeignMesh final : public IMesh
    {
    public:
        /**
         * @brief Report a vertex count, as any mesh must.
         * @return Three, being one triangle's worth.
         */
        [[nodiscard]] std::size_t vertexCount() const override
        {
            return 3;
        }

        /**
         * @brief Report a triangle count, as any mesh must.
         * @return One.
         */
        [[nodiscard]] std::size_t triangleCount() const override
        {
            return 1;
        }
    };

    /**
     * @brief The behaviour every IGfxBackend must share, whichever
     * graphics framework it wraps.
     *
     * Backends under backends/ cannot be held to the coverage gate,
     * because CI has no display and no framework installed. This suite is
     * what replaces that: a backend is finished when it passes this
     * unmodified. Instantiate it with a traits type exposing
     * static std::unique_ptr<IGfxBackend> create(ILogger &):
     *
     * @code
     * INSTANTIATE_TYPED_TEST_SUITE_P(Sdl3, GfxBackendConformance, Traits);
     * @endcode
     *
     * Include this header only from a file that instantiates it, since
     * GoogleTest fails a suite that is registered and never instantiated.
     *
     * What is deliberately *not* asserted here matters as much as what
     * is. Nothing checks the exact size a window reports, or that a fresh
     * queue is empty, because a real window manager is free to resize a
     * window as it appears and to post events nobody asked for. Requiring
     * either would force an honest backend to lie.
     */
    template <typename BackendTraits>
    class GfxBackendConformance : public ::testing::Test
    {
    protected:
        [[nodiscard]] static WindowDesc demoDesc()
        {
            return WindowDesc{
                .title = "Antwika conformance",
                .size = {.width = 640, .height = 480}};
        }

        /**
         * @brief A 4x4 bitmap every backend must be able to upload.
         *
         * Deliberately tiny and opaque grey, because nothing here can
         * read a pixel back to check what became of it.
         */
        [[nodiscard]] static Bitmap demoBitmap()
        {
            constexpr std::uint32_t kSide = 4;

            return Bitmap{
                .size = {.width = kSide, .height = kSide},
                .pixels = std::vector<std::uint8_t>(
                    kSide * kSide * kBytesPerPixel, 128)};
        }

        /**
         * @brief The rectangle covering all of demoBitmap().
         */
        [[nodiscard]] static Rect wholeBitmap()
        {
            return Rect{
                .origin = {.x = 0, .y = 0},
                .size = demoBitmap().size};
        }

        /**
         * @brief Two triangles every backend with a 3D path must be
         * able to upload.
         *
         * Deliberately tiny, indexed, and short of a whole cube,
         * because nothing here can read a vertex back to check what
         * became of it.
         * The four vertices differ in colour and in texture coordinate
         * as well as in position, so a backend that dropped an
         * attribute would still have had to carry it this far.
         */
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

        /**
         * @brief A camera that has demoMesh() well inside its frustum.
         */
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

        /**
         * @brief Reach a window's 3D renderer, or say why there is
         * none.
         *
         * A backend without a 3D path is conforming: IRenderer::
         * renderer3d() returns null by default, precisely so that one
         * need not write no-ops for calls it cannot honour.
         * Every 3D test therefore skips rather than fails.
         * @param window The window whose renderer to ask.
         * @return Its 3D half, or null.
         */
        [[nodiscard]] static IRenderer3D *renderer3dOf(IWindow &window)
        {
            return window.renderer().renderer3d();
        }

        ::testing::NiceMock<MockLogger> logger;
        std::unique_ptr<IGfxBackend> backend{BackendTraits::create(logger)};
    };

    TYPED_TEST_SUITE_P(GfxBackendConformance);

    TYPED_TEST_P(GfxBackendConformance, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ReturnsAnOpenWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_GivesTheWindowARealId)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_NE(window->id(), kNullWindowId);
    }

    TYPED_TEST_P(GfxBackendConformance, MaxWindows_IsAtLeastOne)
    {
        EXPECT_GE(this->backend->maxWindows(), std::size_t{1});
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_GivesEachWindowItsOwnId)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());

        EXPECT_NE(first->id(), second->id());
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_RefusesToExceedItsLimit)
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

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ReportsTheRequestedTitle)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_EQ(window->title(), "Antwika conformance");
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ReportsANonZeroSize)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ThrowsWhenWidthIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowDesc{
                    .title = "Antwika conformance",
                    .size = {.width = 0, .height = 480}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ThrowsWhenHeightIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowDesc{
                    .title = "Antwika conformance",
                    .size = {.width = 640, .height = 0}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_ReturnsIndependentWindows)
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

    TYPED_TEST_P(GfxBackendConformance, SetTitle_ReplacesTheTitle)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->setTitle("Antwika renamed");

        EXPECT_EQ(window->title(), "Antwika renamed");
    }

    TYPED_TEST_P(GfxBackendConformance, Close_ClosesTheWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformance, Close_IsIdempotent)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_NO_THROW(window->close());
        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformance, Renderer_AcceptsAFrameWithoutThrowing)
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

    TYPED_TEST_P(GfxBackendConformance, DrawLine_AcceptsAwkwardLines)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        // Nothing here can be checked by reading pixels back.
        // What a backend must not do is refuse any of it.
        // The zero-length line is the one worth listing first.
        // It is one pixel, not nothing drawn at all.
        // A backend deriving a direction from the ends divides by zero.
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

    TYPED_TEST_P(GfxBackendConformance, DrawText_AcceptsAwkwardText)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        // Nothing here can be checked by reading pixels back.
        // What a backend must not do is refuse any of it.
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

    TYPED_TEST_P(GfxBackendConformance, CreateTexture_ReportsTheBitmapSize)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        const auto texture =
            window->renderer().createTexture(this->demoBitmap());

        ASSERT_NE(texture, nullptr);

        // The one thing a texture is allowed to report.
        // It must be the size handed in, not one the framework chose.
        // Any other answer is the window system reaching the caller.
        EXPECT_EQ(texture->size(), this->demoBitmap().size);
    }

    TYPED_TEST_P(
        GfxBackendConformance, CreateTexture_ThrowsOnAnIncompleteBitmap)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();

        EXPECT_THROW(
            { const auto texture = renderer.createTexture(Bitmap{}); },
            GfxError);

        // A size with no pixels behind it is the likelier mistake.
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
        GfxBackendConformance, DrawTexture_AcceptsAFrameWithoutThrowing)
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

    TYPED_TEST_P(GfxBackendConformance, DrawTexture_AcceptsAnUndrawableBlit)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &renderer = window->renderer();
        const auto texture = renderer.createTexture(this->demoBitmap());
        const auto whole = this->wholeBitmap();
        const Color white{.red = 255, .green = 255, .blue = 255};

        // Nothing here can be checked by reading pixels back.
        // What a backend must not do is refuse any of it.
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
        GfxBackendConformance, DrawTexture_AcceptsATextureFromAnotherRenderer)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());
        const auto texture =
            first->renderer().createTexture(this->demoBitmap());

        // Drawing somebody else's texture must draw nothing.
        // Handing a foreign handle to the framework is the hazard.
        EXPECT_NO_THROW({
            second->renderer().drawTexture(
                *texture, this->wholeBitmap(), this->wholeBitmap(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformance, DrawTexture_AcceptsATextureOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        const ForeignTexture foreign;
        const auto whole = this->wholeBitmap();

        // The test above skips on a single-window backend.
        // That leaves its "is this even mine?" guard unasked.
        // This one needs a single window, so every backend answers.
        EXPECT_NO_THROW({
            window->renderer().drawTexture(
                foreign, whole, whole,
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformance, Texture_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->demoDesc());
        auto texture = window->renderer().createTexture(this->demoBitmap());

        window->close();

        // A closed window's renderer stays reachable.
        // So does a texture made through it.
        EXPECT_NO_THROW(window->renderer().drawTexture(
            *texture, this->wholeBitmap(), this->wholeBitmap(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        // Freeing a texture must not reach a framework that has gone.
        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(texture.reset());
    }

    TYPED_TEST_P(GfxBackendConformance, CreateMesh_ReportsTheGeometrysCounts)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        const auto mesh = renderer->createMesh(this->demoMesh());

        ASSERT_NE(mesh, nullptr);

        // The two things a mesh is allowed to report.
        // They must be what was handed in, not what a framework made.
        // A texture must report its own size for the same reason.
        EXPECT_EQ(mesh->vertexCount(), this->demoMesh().vertices.size());
        EXPECT_EQ(mesh->triangleCount(), this->demoMesh().triangleCount());
    }

    TYPED_TEST_P(GfxBackendConformance, CreateMesh_ThrowsOnIncompleteData)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        EXPECT_THROW(
            { const auto mesh = renderer->createMesh(MeshData{}); },
            GfxError);

        // An index addressing a vertex nobody supplied is likelier.
        // It is also the one a framework reads past the end on.
        EXPECT_THROW(
            {
                const auto mesh = renderer->createMesh(
                    MeshData{
                        .vertices = {Vertex3D{}},
                        .indices = {0, 1, 2}});
            },
            GfxError);

        // So is a count of indices that is not whole triangles.
        EXPECT_THROW(
            {
                const auto mesh = renderer->createMesh(
                    MeshData{
                        .vertices = {Vertex3D{}, Vertex3D{}, Vertex3D{}},
                        .indices = {0, 1}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformance, DrawMesh_AcceptsAFrameWithoutThrowing)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto &flat = window->renderer();
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        const auto mesh = renderer->createMesh(this->demoMesh());
        const auto camera = this->demoCamera();
        const Color white{.red = 255, .green = 255, .blue = 255};

        // Nothing here can be checked by reading a pixel back.
        // What a backend must not do is refuse any of it.
        // The 2D calls after it must still work, too.
        // There is one frame, and both halves draw into it.
        EXPECT_NO_THROW({
            flat.clear(Color{.red = 8, .green = 8, .blue = 16});
            renderer->drawMesh(*mesh, identityMatrix(), camera, white);
            renderer->drawMesh(
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

    TYPED_TEST_P(GfxBackendConformance, DrawMesh_AcceptsAnAwkwardCamera)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        const auto mesh = renderer->createMesh(this->demoMesh());
        const Color white{.red = 255, .green = 255, .blue = 255};

        // An eye on its own target has no direction to look in.
        // An orthographic projection is the other kind entirely.
        // Both must draw rather than divide by zero or be refused.
        EXPECT_NO_THROW({
            renderer->drawMesh(
                *mesh,
                identityMatrix(),
                Camera3D{
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 1.0F, 0.0F},
                    Perspective{}},
                white);
            renderer->drawMesh(
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
        GfxBackendConformance, DrawMesh_AcceptsAMeshFromAnotherRenderer)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second = this->backend->createWindow(this->demoDesc());
        auto *mine = this->renderer3dOf(*first);
        auto *theirs = this->renderer3dOf(*second);

        if (mine == nullptr || theirs == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        const auto mesh = mine->createMesh(this->demoMesh());

        // Drawing somebody else's mesh must draw nothing.
        // Handing a foreign buffer name to the framework is the hazard.
        EXPECT_NO_THROW({
            theirs->drawMesh(
                *mesh,
                identityMatrix(),
                this->demoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformance, DrawMesh_AcceptsAMeshOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->demoDesc());
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        const ForeignMesh foreign;

        // ForeignTexture's question, asked of geometry.
        // A buffer name that was never there is the hazard.
        EXPECT_NO_THROW({
            renderer->drawMesh(
                foreign,
                identityMatrix(),
                this->demoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformance, Mesh_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->demoDesc());
        auto *renderer = this->renderer3dOf(*window);

        if (renderer == nullptr)
        {
            GTEST_SKIP() << "backend offers no 3D renderer";
        }

        auto mesh = renderer->createMesh(this->demoMesh());

        window->close();

        // A closed window's renderer stays reachable.
        // So does a mesh made through it.
        EXPECT_NO_THROW(renderer->drawMesh(
            *mesh,
            identityMatrix(),
            this->demoCamera(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        // Freeing a mesh must not reach a framework that has gone.
        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(mesh.reset());
    }

    TYPED_TEST_P(GfxBackendConformance, PollEvent_DrainsToAnEmptyQueue)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        std::uint32_t polls = 0;

        while (this->backend->pollEvent())
        {
            ++polls;

            ASSERT_LT(polls, kPollLimit)
                << "pollEvent never reported an empty queue";
        }

        SUCCEED();
    }

    TYPED_TEST_P(GfxBackendConformance, PollEvent_DrainsAfterAFrameIsDrawn)
    {
        const auto window = this->backend->createWindow(WindowDesc{
            .title = "Antwika conformance",
            .size = {.width = 640, .height = 480},
            .resizable = true});

        auto &renderer = window->renderer();

        // A backend reading live state must latch what it reported.
        // Otherwise the same state comes back on every poll.
        // Presenting is when such a backend looks at the window system.
        for (std::uint32_t frame = 0; frame < 3; ++frame)
        {
            renderer.clear(Color{.red = 8, .green = 8, .blue = 8});
            renderer.present();

            std::uint32_t polls = 0;

            while (this->backend->pollEvent())
            {
                ++polls;

                ASSERT_LT(polls, kPollLimit)
                    << "pollEvent never reported an empty queue";
            }
        }

        SUCCEED();
    }

    TYPED_TEST_P(GfxBackendConformance, Window_MayOutliveItsBackend)
    {
        auto window = this->backend->createWindow(this->demoDesc());

        this->backend.reset();

        // Destroying the window must not reach back into its backend.
        EXPECT_NO_THROW(window.reset());
    }

    REGISTER_TYPED_TEST_SUITE_P(
        GfxBackendConformance,
        Name_IsNotEmpty,
        MaxWindows_IsAtLeastOne,
        CreateWindow_ReturnsAnOpenWindow,
        CreateWindow_GivesTheWindowARealId,
        CreateWindow_GivesEachWindowItsOwnId,
        CreateWindow_RefusesToExceedItsLimit,
        CreateWindow_ReportsTheRequestedTitle,
        CreateWindow_ReportsANonZeroSize,
        CreateWindow_ThrowsWhenWidthIsZero,
        CreateWindow_ThrowsWhenHeightIsZero,
        CreateWindow_ReturnsIndependentWindows,
        SetTitle_ReplacesTheTitle,
        Close_ClosesTheWindow,
        Close_IsIdempotent,
        Renderer_AcceptsAFrameWithoutThrowing,
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

} // namespace antwika::gfx::conformance
