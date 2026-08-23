#pragma once

#include <glm/gtc/matrix_transform.hpp>
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
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/gfx/Transform.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/fakes/FakeForeignMesh.hpp>
#include <antwika/gfx/fakes/FakeForeignShader.hpp>
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
        [[nodiscard]] static WindowSpec getDemoSpec()
        {
            return WindowSpec{
                .title = "Antwika conformance",
                .size = {.width = 640, .height = 480},
                .hidden = true};
        }

        [[nodiscard]] static WindowSpec getResizableSpec()
        {
            return WindowSpec{
                .title = "Antwika conformance, resizable",
                .size = {.width = 320, .height = 240},
                .resizable = true,
                .hidden = true};
        }

        [[nodiscard]] static WindowSpec getFullscreenSpec()
        {
            return WindowSpec{
                .title = "Antwika conformance, fullscreen",
                .size = {.width = 480, .height = 360},
                .fullscreen = true,
                .hidden = true};
        }

        [[nodiscard]] static Bitmap getDemoBitmap()
        {
            constexpr std::uint32_t kSide = 4;

            return Bitmap{
                .size = {.width = kSide, .height = kSide},
                .pixels = std::vector<std::uint8_t>(
                    kSide * kSide * kBytesPerPixel, 128)};
        }

        [[nodiscard]] static Rect getWholeBitmap()
        {
            return Rect{
                .originPoint = {.x = 0, .y = 0},
                .size = getDemoBitmap().size};
        }

        [[nodiscard]] static MeshData getDemoMesh()
        {
            return MeshData{
                .vertices =
                    {Vertex3D{
                         .position = {-1.0F, -1.0F, 0.0F},
                         .texCoordinate = {0.0F, 0.0F},
                         .color = {.red = 255}},
                     Vertex3D{
                         .position = {1.0F, -1.0F, 0.0F},
                         .texCoordinate = {1.0F, 0.0F},
                         .color = {.green = 255}},
                     Vertex3D{
                         .position = {1.0F, 1.0F, 0.0F},
                         .texCoordinate = {1.0F, 1.0F},
                         .color = {.blue = 255}},
                     Vertex3D{
                         .position = {-1.0F, 1.0F, 0.0F},
                         .texCoordinate = {0.0F, 1.0F},
                         .color = {.red = 255, .green = 255}}},
                .indices = {0, 1, 2, 0, 2, 3}};
        }

        [[nodiscard]] static ShaderSource getDemoShader()
        {
            return ShaderSource{
                .vertex = R"(#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)",
                .fragment = R"(#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main()
{
    finalColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
}
)"};
        }

        [[nodiscard]] static MeshData getFlatMesh()
        {
            auto mesh = getDemoMesh();

            for (auto &vertex : mesh.vertices)
            {
                vertex.color = Color{
                    .red = 255,
                    .green = 255,
                    .blue = 255,
                    .alpha = 255};
            }

            return mesh;
        }

        [[nodiscard]] static Mat4 getMovedBy(const float z)
        {
            auto matrix = getIdentityMatrix();

            matrix[3][2] = z;

            return matrix;
        }

        [[nodiscard]] static Mat4 getMovedAcross(const float x)
        {
            auto matrix = getIdentityMatrix();

            matrix[3][0] = x;

            return matrix;
        }

        [[nodiscard]] static Bitmap bitmapOf(const Color color)
        {
            auto bitmap = getDemoBitmap();

            for (std::size_t index = 0; index + 3 < bitmap.pixels.size();
                 index += kBytesPerPixel)
            {
                bitmap.pixels[index] = color.red;
                bitmap.pixels[index + 1] = color.green;
                bitmap.pixels[index + 2] = color.blue;
                bitmap.pixels[index + 3] = color.alpha;
            }

            return bitmap;
        }

        [[nodiscard]] static std::optional<Color> middleOf(
            IRenderer &renderer)
        {
            const Bitmap takenBitmap = renderer.readPixels();

            if (takenBitmap.pixels.empty() || !takenBitmap.isValid())
            {
                return std::nullopt;
            }

            const auto byteIndex = (static_cast<std::size_t>(
                                 takenBitmap.size.height / 2)
                                 * takenBitmap.size.width
                             + takenBitmap.size.width / 2)
                            * kBytesPerPixel;

            return Color{
                .red = takenBitmap.pixels[byteIndex],
                .green = takenBitmap.pixels[byteIndex + 1],
                .blue = takenBitmap.pixels[byteIndex + 2],
                .alpha = takenBitmap.pixels[byteIndex + 3]};
        }

        [[nodiscard]] static ShaderSource getSurfaceShader()
        {
            return ShaderSource{
                .vertex = getDemoShader().vertex,
                .fragment = R"(#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform vec4 colDiffuse;

void main()
{
    vec4 texel = texture(texture0, fragTexCoord);
    vec4 surface = texture(texture1, fragTexCoord);

    finalColor = vec4(vec3(surface.r), texel.a) * colDiffuse;
}
)"};
        }

        [[nodiscard]] static ShaderSource getShiftingShader()
        {
            return ShaderSource{
                .vertex = R"(#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec2 fragTexCoord;
out vec4 fragColor;

uniform mat4 mvp;
uniform mat4 shift;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    gl_Position = mvp * shift * vec4(vertexPosition, 1.0);
}
)",
                .fragment = getDemoShader().fragment};
        }

        [[nodiscard]] static ShaderSource getDepthShader()
        {
            return ShaderSource{
                .vertex = getDemoShader().vertex,
                .fragment = R"(#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform sampler2D texture0;

void main()
{
    float held = texture(texture0, fragTexCoord).r;

    finalColor = vec4(vec3(held), 1.0);
}
)"};
        }

        [[nodiscard]] static Camera3D getDemoCamera()
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
        EXPECT_FALSE(this->backend->getName().empty());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ReturnsAnOpenWindow)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_GivesTheWindowARealId)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        EXPECT_NE(window->getId(), kNullWindowId);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, MaxWindows_IsAtLeastOne)
    {
        EXPECT_GE(this->backend->getMaxWindows(), std::size_t{1});
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_GivesEachWindowItsOwnId)
    {
        if (this->backend->getMaxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());
        const auto second = this->backend->createWindow(this->getDemoSpec());

        EXPECT_NE(first->getId(), second->getId());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_RefusesToExceedItsLimit)
    {
        if (this->backend->getMaxWindows() != 1)
        {
            GTEST_SKIP() << "backend allows more than one window";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());

        EXPECT_THROW(
            {
                const auto second =
                    this->backend->createWindow(this->getDemoSpec());
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_ReportsTheRequestedTitle)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        EXPECT_EQ(window->getTitle(), "Antwika conformance");
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ReportsANonZeroSize)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        EXPECT_GT(window->getSize().width, 0u);
        EXPECT_GT(window->getSize().height, 0u);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, ConfiguredSize_IsExactlyWhatWasAskedFor)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        EXPECT_EQ(window->getConfiguredSize(), this->getDemoSpec().size);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, ConfiguredSize_IsUnchangedByClosingTheWindow)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        window->close();

        EXPECT_EQ(window->getConfiguredSize(), this->getDemoSpec().size);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, ConfiguredSize_IsPerWindow)
    {
        if (this->backend->getMaxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());
        const auto second =
            this->backend->createWindow(this->getResizableSpec());

        EXPECT_EQ(first->getConfiguredSize(), this->getDemoSpec().size);
        EXPECT_EQ(second->getConfiguredSize(), this->getResizableSpec().size);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_AcceptsAResizableWindow)
    {
        const auto window =
            this->backend->createWindow(this->getResizableSpec());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());

        EXPECT_EQ(window->getConfiguredSize(), this->getResizableSpec().size);
        EXPECT_GT(window->getSize().width, 0u);
        EXPECT_GT(window->getSize().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_AcceptsAFullscreenWindow)
    {
        const auto window =
            this->backend->createWindow(this->getFullscreenSpec());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());

        EXPECT_EQ(window->getConfiguredSize(), this->getFullscreenSpec().size);
        EXPECT_GT(window->getSize().width, 0u);
        EXPECT_GT(window->getSize().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetFullscreen_LeavesTheWindowUsable)
    {
        const auto window =
            this->backend->createWindow(this->getResizableSpec());

        window->setFullscreen(true);
        window->setFullscreen(true);
        window->setFullscreen(false);

        EXPECT_EQ(window->getConfiguredSize(), this->getResizableSpec().size);
        EXPECT_GT(window->getSize().width, 0u);
        EXPECT_GT(window->getSize().height, 0u);
        EXPECT_TRUE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetFullscreen_IsHarmlessOnceClosed)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        window->close();

        EXPECT_NO_THROW(window->setFullscreen(true));
        EXPECT_EQ(window->getConfiguredSize(), this->getDemoSpec().size);
        EXPECT_GT(window->getSize().width, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Size_StaysNonZeroAfterClosing)
    {
        const auto window =
            this->backend->createWindow(this->getResizableSpec());

        window->close();

        EXPECT_GT(window->getSize().width, 0u);
        EXPECT_GT(window->getSize().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ThrowsWhenWidthIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowSpec{
                    .title = "Antwika conformance",
                    .size = {.width = 0, .height = 480}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateWindow_ThrowsWhenHeightIsZero)
    {
        EXPECT_THROW(
            {
                const auto window = this->backend->createWindow(WindowSpec{
                    .title = "Antwika conformance",
                    .size = {.width = 640, .height = 0}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateWindow_ReturnsIndependentWindows)
    {
        if (this->backend->getMaxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());
        const auto second = this->backend->createWindow(this->getDemoSpec());

        first->close();

        EXPECT_FALSE(first->isOpen());
        EXPECT_TRUE(second->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetSize_ResizesTheWindow)
    {
        if (!this->backend->getCapabilities().resizesWindows)
        {
            GTEST_SKIP() << "the backend does not resize windows";
        }

        const auto window = this->backend->createWindow(this->getDemoSpec());
        const Size askedSize{.width = 320, .height = 240};

        window->setSize(askedSize);

        EXPECT_EQ(window->getSize(), askedSize);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetTitle_ReplacesTheTitle)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        window->setTitle("Antwika renamed");

        EXPECT_EQ(window->getTitle(), "Antwika renamed");
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Close_ClosesTheWindow)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        window->close();

        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Close_IsIdempotent)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

        window->close();

        EXPECT_NO_THROW(window->close());
        EXPECT_FALSE(window->isOpen());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 Renderer_AcceptsAFrameWithoutThrowing)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.clear(Color{.red = 8, .green = 8, .blue = 8});
            renderer.drawRect(
                Rect{
                    .originPoint = {.x = 1, .y = 2},
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
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = window->renderer();

        EXPECT_NO_THROW({
            renderer.clear(Color{.red = 200});
            renderer.drawRect(
                Rect{
                    .originPoint = {.x = 4, .y = 4},
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
                    .originPoint = {.x = 8, .y = 8},
                    .size = {.width = 8, .height = 8}},
                Color{.red = 255, .alpha = 0});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawLine_AcceptsAwkwardLines)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
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
        const auto window = this->backend->createWindow(this->getDemoSpec());
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
        const auto window = this->backend->createWindow(this->getDemoSpec());
        const auto texture =
            window->renderer().createTexture(this->getDemoBitmap());

        ASSERT_NE(texture, nullptr);

        EXPECT_EQ(texture->getSize(), this->getDemoBitmap().size);
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, CreateTexture_ThrowsOnAnIncompleteBitmap)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
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
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = window->renderer();
        const auto texture = renderer.createTexture(this->getDemoBitmap());
        const auto bitmap = this->getWholeBitmap();

        EXPECT_NO_THROW({
            renderer.clear(Color{});
            renderer.drawTexture(
                *texture, bitmap,
                Rect{
                    .originPoint = {.x = 8, .y = 8},
                    .size = {.width = 4, .height = 4}},
                Color{.red = 255, .green = 255, .blue = 255});
            renderer.drawTexture(
                *texture,
                Rect{
                    .originPoint = {.x = 1, .y = 1},
                    .size = {.width = 2, .height = 2}},
                Rect{
                    .originPoint = {.x = 40, .y = 40},
                    .size = {.width = 64, .height = 64}},
                Color{.red = 255, .green = 80, .blue = 80, .alpha = 128});
            renderer.drawTexture(
                *texture, bitmap,
                Rect{
                    .originPoint = {.x = -20, .y = -20},
                    .size = {.width = 32, .height = 32}},
                Color{.red = 255, .green = 255, .blue = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawTexture_AcceptsAnUndrawableBlit)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = window->renderer();
        const auto texture = renderer.createTexture(this->getDemoBitmap());
        const auto bitmap = this->getWholeBitmap();
        const Color whiteColor{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            renderer.drawTexture(*texture, Rect{}, bitmap, whiteColor);
            renderer.drawTexture(*texture, bitmap, Rect{}, whiteColor);
            renderer.drawTexture(
                *texture,
                Rect{
                    .originPoint = {.x = -1, .y = -1},
                    .size = {.width = 4, .height = 4}},
                bitmap, whiteColor);
            renderer.drawTexture(
                *texture,
                Rect{
                    .originPoint = {.x = 2, .y = 2},
                    .size = {.width = 99, .height = 99}},
                bitmap, whiteColor);
            renderer.present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest,
        DrawTexture_AcceptsATextureFromAnotherRenderer)
    {
        if (this->backend->getMaxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());
        const auto second = this->backend->createWindow(this->getDemoSpec());
        const auto texture =
            first->renderer().createTexture(this->getDemoBitmap());

        EXPECT_NO_THROW({
            second->renderer().drawTexture(
                *texture, this->getWholeBitmap(), this->getWholeBitmap(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawTexture_AcceptsATextureOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        const fakes::FakeForeignTexture foreign;
        const auto bitmap = this->getWholeBitmap();

        EXPECT_NO_THROW({
            window->renderer().drawTexture(
                foreign, bitmap, bitmap,
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Texture_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->getDemoSpec());
        auto texture = window->renderer().createTexture(this->getDemoBitmap());

        window->close();

        EXPECT_NO_THROW(window->renderer().drawTexture(
            *texture, this->getWholeBitmap(), this->getWholeBitmap(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(texture.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateMesh_ReportsTheGeometrysCounts)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());

        ASSERT_NE(mesh, nullptr);

        EXPECT_EQ(mesh->getVertexCount(), this->getDemoMesh().vertices.size());
        EXPECT_EQ(mesh->getTriangleCount(), this->getDemoMesh().getTriangleCount());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateMesh_ThrowsOnIncompleteData)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
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
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &windowRenderer = window->renderer();
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());
        const auto camera = this->getDemoCamera();
        const Color whiteColor{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            windowRenderer.clear(Color{.red = 8, .green = 8, .blue = 16});
            renderer.drawMesh(*mesh, getIdentityMatrix(), camera, whiteColor);
            renderer.drawMesh(
                *mesh,
                Transform{
                    .position = {0.5F, 0.0F, -1.0F},
                    .rotationRadians = {0.3F, 0.7F, 0.0F},
                    .scale = {0.5F, 0.5F, 0.5F}}
                    .getMatrix(),
                camera,
                Color{.red = 255, .green = 128, .blue = 0, .alpha = 128});
            windowRenderer.drawRect(
                Rect{
                    .originPoint = {.x = 4, .y = 4},
                    .size = {.width = 16, .height = 16}},
                Color{.green = 255});
            windowRenderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawMesh_AcceptsAnAwkwardCamera)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());
        const Color whiteColor{.red = 255, .green = 255, .blue = 255};

        EXPECT_NO_THROW({
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                Camera3D{
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 1.0F, 0.0F},
                    Perspective{}},
                whiteColor);
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                Camera3D{
                    Vec3{2.0F, 2.0F, 2.0F},
                    Vec3{0.0F, 0.0F, 0.0F},
                    Vec3{0.0F, 1.0F, 0.0F},
                    Orthographic{.halfWidth = 3.0F, .halfHeight = 2.0F}},
                whiteColor);
            window->renderer().present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawMesh_AcceptsAMeshFromAnotherRenderer)
    {
        if (this->backend->getMaxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->getDemoSpec());
        const auto second = this->backend->createWindow(this->getDemoSpec());
        auto &mine = this->rendererOf(*first);
        auto &theirs = this->rendererOf(*second);

        const auto mesh = mine.createMesh(this->getDemoMesh());

        EXPECT_NO_THROW({
            theirs.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            second->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawMesh_AcceptsAMeshOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const fakes::FakeForeignMesh foreign;

        EXPECT_NO_THROW({
            renderer.drawMesh(
                foreign,
                getIdentityMatrix(),
                this->getDemoCamera(),
                Color{.red = 255, .green = 255, .blue = 255});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Mesh_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        auto mesh = renderer.createMesh(this->getDemoMesh());

        window->close();

        EXPECT_NO_THROW(renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            Color{.red = 255, .green = 255, .blue = 255}));

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(mesh.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateShader_ReturnsAReadyProgram)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto shader = renderer.createShader(this->getDemoShader());

        ASSERT_NE(shader, nullptr);

        EXPECT_TRUE(shader->isReady());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, CreateShader_ThrowsOnAMissingStage)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        EXPECT_THROW(
            { const auto shader = renderer.createShader(ShaderSource{}); },
            GfxError);

        EXPECT_THROW(
            {
                const auto shader = renderer.createShader(
                    ShaderSource{
                        .vertex = this->getDemoShader().vertex,
                        .fragment = ""});
            },
            GfxError);

        EXPECT_THROW(
            {
                const auto shader = renderer.createShader(
                    ShaderSource{
                        .vertex = "",
                        .fragment = this->getDemoShader().fragment});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, DrawMesh_AcceptsAFullMaterial)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());
        const auto texture = renderer.createTexture(this->getDemoBitmap());
        const auto shader = renderer.createShader(this->getDemoShader());

        EXPECT_NO_THROW({
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                MeshMaterial{
                    .texture = texture.get(),
                    .shader = shader.get(),
                    .tintColor = {.red = 255, .green = 128, .blue = 0}});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_LeavesLaterFlatDrawingUnshaded)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());
        const auto shader = renderer.createShader(this->getDemoShader());

        EXPECT_NO_THROW({
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                MeshMaterial{.shader = shader.get()});
            renderer.drawRect(
                Rect{
                    .originPoint = {.x = 4, .y = 4},
                    .size = {.width = 16, .height = 16}},
                Color{.green = 255});
            renderer.drawText(
                Point{.x = 8, .y = 8}, "over", 1, Color{.blue = 255});
            renderer.present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, DrawMesh_AcceptsAShaderOfAnotherKind)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getDemoMesh());
        const fakes::FakeForeignShader foreign;
        const fakes::FakeForeignTexture foreignTexture;

        EXPECT_NO_THROW({
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                MeshMaterial{
                    .texture = &foreignTexture, .shader = &foreign});
            window->renderer().present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, Shader_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        auto mesh = renderer.createMesh(this->getDemoMesh());
        auto shader = renderer.createShader(this->getDemoShader());

        window->close();

        EXPECT_NO_THROW(renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{.shader = shader.get()}));

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(shader.reset());
        EXPECT_NO_THROW(mesh.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetShader_TakesEveryKindOfValue)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto shader = renderer.createShader(this->getDemoShader());
        const auto mesh = renderer.createMesh(this->getDemoMesh());

        EXPECT_NO_THROW({
            renderer.setShaderNumber(*shader, "colDiffuse", 0.5F);
            renderer.setShaderVector(
                *shader, "colDiffuse", Vec3{1.0F, 0.0F, 0.0F});
            renderer.setShaderColor(
                *shader, "colDiffuse", Color{.red = 255});
            renderer.setShaderMatrix(
                *shader, "mvp", getIdentityMatrix());
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                MeshMaterial{.shader = shader.get()});
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, SetShader_IgnoresAUniformItHasNot)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto shader = renderer.createShader(this->getDemoShader());
        const fakes::FakeForeignShader foreign;

        EXPECT_NO_THROW({
            renderer.setShaderNumber(*shader, "nothingByThatName", 1.0F);
            renderer.setShaderVector(
                foreign, "colDiffuse", Vec3{1.0F, 1.0F, 1.0F});
            renderer.setShaderColor(
                foreign, "colDiffuse", Color{.red = 255});
            renderer.setShaderMatrix(
                *shader, "nothingByThatName", getIdentityMatrix());
            renderer.setShaderMatrix(
                foreign, "mvp", getIdentityMatrix());
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateRenderTarget_KeepsDepthOnlyWhenAskedTo)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const Size wantedSize{.width = 64, .height = 32};

        const auto plain =
            renderer.createRenderTarget(RenderTargetSpec{
                .size = wantedSize, .depth = false});
        const auto depthTarget = renderer.createRenderTarget(
            RenderTargetSpec{.size = wantedSize, .depth = true});

        EXPECT_EQ(plain->getSize().width, wantedSize.width);
        EXPECT_EQ(plain->getSize().height, wantedSize.height);
        EXPECT_NE(plain->getColor(), nullptr);
        EXPECT_EQ(plain->getDepth(), nullptr);

        EXPECT_NE(depthTarget->getColor(), nullptr);
        ASSERT_NE(depthTarget->getDepth(), nullptr);
        EXPECT_EQ(depthTarget->getDepth()->getSize().width, wantedSize.width);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 CreateRenderTarget_ThrowsOnAnEmptySize)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        EXPECT_THROW(
            {
                (void)renderer.createRenderTarget(
                    RenderTargetSpec{
                        .size = Size{.width = 0, .height = 8}});
            },
            GfxError);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 BeginTarget_KeepsWhatWasDrawnIntoIt)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto target = renderer.createRenderTarget(
            RenderTargetSpec{.size = Size{.width = 64, .height = 64}});
        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader = renderer.createShader(this->getDemoShader());

        renderer.beginTarget(*target);
        renderer.clear(Color{.red = 0, .green = 255, .blue = 0});
        renderer.endTarget();

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = target->getColor(), .shader = shader.get()});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->green, 255, 8);
        EXPECT_NEAR(middleColor->blue, 0, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 BeginTargetRegion_LeavesTheRestOfTheTargetAsItStood)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto target = renderer.createRenderTarget(
            RenderTargetSpec{.size = Size{.width = 64, .height = 64}});
        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader = renderer.createShader(this->getDemoShader());

        renderer.beginTarget(*target);
        renderer.clear(Color{.red = 0, .green = 255, .blue = 0});
        renderer.endTarget();

        renderer.beginTargetRegion(
            *target,
            Rect{
                .originPoint = {.x = 0, .y = 0},
                .size = {.width = 8, .height = 8}});
        renderer.clear(Color{.red = 255, .green = 0, .blue = 0});
        renderer.endTarget();

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = target->getColor(), .shader = shader.get()});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->green, 255, 8);
        EXPECT_NEAR(middleColor->red, 0, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 BeginTarget_KeepsDepthAsATextureAPassMaySample)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto target = renderer.createRenderTarget(
            RenderTargetSpec{
                .size = Size{.width = 64, .height = 64},
                .depth = true});

        ASSERT_NE(target->getDepth(), nullptr);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto plain = renderer.createShader(this->getDemoShader());
        const auto showing = renderer.createShader(this->getDepthShader());

        const auto readBack = [&]
        {
            renderer.clear(Color{.red = 0, .green = 0, .blue = 0});
            renderer.drawMesh(
                *mesh,
                getIdentityMatrix(),
                this->getDemoCamera(),
                MeshMaterial{
                    .texture = target->getDepth(),
                    .shader = showing.get()});

            const auto middleColor = this->middleOf(renderer);

            renderer.present();

            return middleColor;
        };

        renderer.beginTarget(*target);
        renderer.clear(Color{.red = 0, .green = 0, .blue = 0});
        renderer.endTarget();

        const auto emptyBitmap = readBack();

        renderer.beginTarget(*target);
        renderer.clear(Color{.red = 0, .green = 0, .blue = 0});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{.shader = plain.get()});
        renderer.endTarget();

        const auto readBitmap = readBack();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(emptyBitmap.has_value());
        ASSERT_TRUE(readBitmap.has_value());

        EXPECT_EQ(emptyBitmap->red, 255);
        EXPECT_LT(readBitmap->red, 250);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 RenderTarget_MayOutliveItsWindow)
    {
        auto window = this->backend->createWindow(this->getDemoSpec());
        auto target = window->renderer().createRenderTarget(
            RenderTargetSpec{
                .size = Size{.width = 32, .height = 32},
                .depth = true});

        window->close();

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
        EXPECT_NO_THROW(target.reset());
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 SetShaderMatrix_MovesWhatTheShaderDraws)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader =
            renderer.createShader(this->getShiftingShader());
        const auto paint = renderer.createTexture(
            this->bitmapOf(Color{.red = 0, .green = 255, .blue = 0}));

        const MeshMaterial surfacingMaterial{
            .texture = paint.get(), .shader = shader.get()};

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.setShaderMatrix(*shader, "shift", getIdentityMatrix());
        renderer.drawMesh(
            *mesh, getIdentityMatrix(), this->getDemoCamera(), surfacingMaterial);

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.setShaderMatrix(
            *shader, "shift", this->getMovedAcross(8.0F));
        renderer.drawMesh(
            *mesh, getIdentityMatrix(), this->getDemoCamera(), surfacingMaterial);

        const auto shiftedColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());
        ASSERT_TRUE(shiftedColor.has_value());

        EXPECT_NEAR(middleColor->green, 255, 8);
        EXPECT_NEAR(shiftedColor->blue, 255, 8);
        EXPECT_NEAR(shiftedColor->green, 0, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_BlendsATranslucentDrawOverTheFrame)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader = renderer.createShader(this->getDemoShader());
        const auto texture = renderer.createTexture(this->bitmapOf(
            Color{.red = 255, .green = 0, .blue = 0, .alpha = 128}));

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = texture.get(),
                .shader = shader.get(),
                .blend = BlendMode::Alpha});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->red, 128, 8);
        EXPECT_NEAR(middleColor->blue, 127, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_KeepsWritingDepthAfterABlendedDraw)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader = renderer.createShader(this->getDemoShader());
        const auto veiledTexture = renderer.createTexture(this->bitmapOf(
            Color{.red = 255, .green = 0, .blue = 0, .alpha = 128}));
        const auto nearTexture = renderer.createTexture(this->bitmapOf(
            Color{.red = 0, .green = 255, .blue = 0, .alpha = 255}));
        const auto farTexture = renderer.createTexture(this->bitmapOf(
            Color{.red = 255, .green = 0, .blue = 255, .alpha = 255}));

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = veiledTexture.get(),
                .shader = shader.get(),
                .blend = BlendMode::Alpha});
        renderer.drawMesh(
            *mesh,
            this->getMovedBy(1.0F),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = nearTexture.get(), .shader = shader.get()});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = farTexture.get(), .shader = shader.get()});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->green, 255, 8);
        EXPECT_NEAR(middleColor->red, 0, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_ReadsAPlainSurfaceWithNoMapBound)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader =
            renderer.createShader(this->getSurfaceShader());
        const auto texture = renderer.createTexture(this->bitmapOf(
            Color{.red = 255, .green = 255, .blue = 255, .alpha = 255}));

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = texture.get(), .shader = shader.get()});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->red, 0, 8);
        EXPECT_NEAR(middleColor->green, 0, 8);
        EXPECT_NEAR(middleColor->blue, 0, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest,
                 DrawMesh_SamplesTheMaterialMapItIsGiven)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        const auto mesh = renderer.createMesh(this->getFlatMesh());
        const auto shader =
            renderer.createShader(this->getSurfaceShader());
        const auto texture = renderer.createTexture(this->bitmapOf(
            Color{.red = 255, .green = 255, .blue = 255, .alpha = 255}));
        const auto surface = renderer.createTexture(this->bitmapOf(
            Color{.red = 200, .green = 0, .blue = 0, .alpha = 255}));

        renderer.clear(Color{.red = 0, .green = 0, .blue = 255});
        renderer.drawMesh(
            *mesh,
            getIdentityMatrix(),
            this->getDemoCamera(),
            MeshMaterial{
                .texture = texture.get(),
                .materialMapTexture = surface.get(),
                .shader = shader.get()});

        const auto middleColor = this->middleOf(renderer);

        renderer.present();

        if (!this->backend->getCapabilities().readsPixels)
        {
            GTEST_SKIP() << "the backend reads no pixels back";
        }

        ASSERT_TRUE(middleColor.has_value());

        EXPECT_NEAR(middleColor->red, 200, 8);
    }

    TYPED_TEST_P(GfxBackendConformanceTest, ReadPixels_ComesBackComplete)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        renderer.clear(Color{.red = 8, .green = 16, .blue = 32});

        const Bitmap takenBitmap = renderer.readPixels();

        EXPECT_TRUE(
            takenBitmap.isValid() || takenBitmap.pixels.empty());

        renderer.present();
    }

    TYPED_TEST_P(GfxBackendConformanceTest, PollEvent_DrainsToAnEmptyQueue)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());

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
        const auto window = this->backend->createWindow(WindowSpec{
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
        auto window = this->backend->createWindow(this->getDemoSpec());

        this->backend.reset();

        EXPECT_NO_THROW(window.reset());
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, PushTransform_AcceptsAFrameDrawnUnderIt)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        EXPECT_NO_THROW({
            renderer.pushTransform(
                glm::translate(getIdentityMatrix(), Vec3{8.0F, 4.0F, 0.0F}));
            renderer.drawRect(
                RectF{PointF{0.0F, 0.0F}, SizeF{16.0F, 16.0F}},
                Color{.red = 255, .green = 255, .blue = 255});
            renderer.popTransform();
            renderer.present();
        });
    }

    TYPED_TEST_P(GfxBackendConformanceTest, PushTransform_Nests)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        EXPECT_NO_THROW({
            renderer.pushTransform(
                glm::scale(getIdentityMatrix(), Vec3{2.0F, 2.0F, 1.0F}));
            renderer.pushTransform(
                glm::translate(getIdentityMatrix(), Vec3{1.0F, 1.0F, 0.0F}));
            renderer.popTransform();
            renderer.popTransform();
            renderer.present();
        });
    }

    TYPED_TEST_P(
        GfxBackendConformanceTest, PopTransform_ThrowsWhenNothingIsPushed)
    {
        const auto window = this->backend->createWindow(this->getDemoSpec());
        auto &renderer = this->rendererOf(*window);

        EXPECT_THROW(renderer.popTransform(), GfxError);
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
        SetSize_ResizesTheWindow,
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
        CreateShader_ReturnsAReadyProgram,
        CreateShader_ThrowsOnAMissingStage,
        DrawMesh_AcceptsAFullMaterial,
        CreateRenderTarget_KeepsDepthOnlyWhenAskedTo,
        CreateRenderTarget_ThrowsOnAnEmptySize,
        BeginTarget_KeepsWhatWasDrawnIntoIt,
        BeginTarget_KeepsDepthAsATextureAPassMaySample,
        BeginTargetRegion_LeavesTheRestOfTheTargetAsItStood,
        RenderTarget_MayOutliveItsWindow,
        SetShaderMatrix_MovesWhatTheShaderDraws,
        DrawMesh_BlendsATranslucentDrawOverTheFrame,
        DrawMesh_KeepsWritingDepthAfterABlendedDraw,
        DrawMesh_ReadsAPlainSurfaceWithNoMapBound,
        DrawMesh_SamplesTheMaterialMapItIsGiven,
        DrawMesh_LeavesLaterFlatDrawingUnshaded,
        DrawMesh_AcceptsAShaderOfAnotherKind,
        Shader_MayOutliveItsWindow,
        SetShader_TakesEveryKindOfValue,
        SetShader_IgnoresAUniformItHasNot,
        ReadPixels_ComesBackComplete,
        PushTransform_AcceptsAFrameDrawnUnderIt,
        PushTransform_Nests,
        PopTransform_ThrowsWhenNothingIsPushed,
        Mesh_MayOutliveItsWindow,
        PollEvent_DrainsToAnEmptyQueue,
        PollEvent_DrainsAfterAFrameIsDrawn,
        Window_MayOutliveItsBackend);

}
