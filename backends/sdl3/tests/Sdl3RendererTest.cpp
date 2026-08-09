#include <SDL3/SDL.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "Sdl3Renderer.hpp"
#include "Sdl3Backend.hpp"

namespace antwika::gfx::sdl3
{

    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    namespace
    {
        class Sdl3RendererTest : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                window = backend.createWindow(WindowDesc{
                    .title = "Antwika blending",
                    .size = {.width = 64, .height = 64}});

                SDL_Window *raw =
                    SDL_GetWindowFromID(static_cast<SDL_WindowID>(
                        rawValue(window->id())));

                ASSERT_NE(raw, nullptr) << SDL_GetError();

                sdlRenderer = SDL_GetRenderer(raw);

                ASSERT_NE(sdlRenderer, nullptr) << SDL_GetError();
            }

            [[nodiscard]] SDL_BlendMode blendMode() const
            {
                SDL_BlendMode mode = SDL_BLENDMODE_INVALID;

                EXPECT_TRUE(SDL_GetRenderDrawBlendMode(sdlRenderer, &mode))
                    << SDL_GetError();

                return mode;
            }

            [[nodiscard]] std::optional<Color> pixelAt(Point at) const
            {
                const SDL_Rect one{.x = at.x, .y = at.y, .w = 1, .h = 1};

                SDL_Surface *read =
                    SDL_RenderReadPixels(sdlRenderer, &one);

                if (read == nullptr)
                {
                    return std::nullopt;
                }

                Color color{};

                const bool got = SDL_ReadSurfacePixel(
                    read,
                    0,
                    0,
                    &color.red,
                    &color.green,
                    &color.blue,
                    &color.alpha);

                SDL_DestroySurface(read);

                return got ? std::optional<Color>{color} : std::nullopt;
            }

            NiceMock<MockLogger> logger;
            Sdl3Backend backend{logger};
            std::unique_ptr<IWindow> window;
            SDL_Renderer *sdlRenderer = nullptr;
        };

        TEST_F(Sdl3RendererTest, DrawRect_BlendsRatherThanOverwriting)
        {
            window->renderer().drawRect(
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = {.width = 4, .height = 4}},
                Color{.red = 0, .green = 0, .blue = 255, .alpha = 48});

            EXPECT_EQ(blendMode(), SDL_BLENDMODE_BLEND);
        }

        TEST_F(Sdl3RendererTest, DrawLine_BlendsRatherThanOverwriting)
        {
            window->renderer().drawLine(
                Point{.x = 0, .y = 0},
                Point{.x = 8, .y = 8},
                Color{.red = 255, .alpha = 128});

            EXPECT_EQ(blendMode(), SDL_BLENDMODE_BLEND);
        }

        TEST_F(Sdl3RendererTest, DrawText_LeavesWhatIsUnderneathShowing)
        {
            constexpr Color kUnder{
                .red = 200, .green = 0, .blue = 0, .alpha = 255};
            constexpr Color kInk{
                .red = 0, .green = 255, .blue = 0, .alpha = 255};

            auto &renderer = window->renderer();

            renderer.clear(kUnder);
            renderer.drawText(Point{.x = 0, .y = 0}, "l", 4, kInk);

            std::optional<Color> blended;
            bool readable = false;

            for (std::int32_t y = 0; y < 32 && !blended.has_value(); ++y)
            {
                for (std::int32_t x = 0; x < 24; ++x)
                {
                    const auto pixel = pixelAt(Point{.x = x, .y = y});

                    if (!pixel.has_value())
                    {
                        break;
                    }

                    readable = true;

                    if (pixel->green > 0 && pixel->red > 0)
                    {
                        blended = pixel;
                        break;
                    }
                }
            }

            if (!readable)
            {
                GTEST_SKIP() << "this driver reports no pixels back";
            }

            ASSERT_TRUE(blended.has_value());

            EXPECT_LT(blended->green, 255);
            EXPECT_LT(blended->red, 200);
        }

        TEST_F(Sdl3RendererTest, Clear_ReplacesRatherThanBlending)
        {
            window->renderer().clear(Color{.red = 8, .green = 8, .blue = 8});

            EXPECT_EQ(blendMode(), SDL_BLENDMODE_NONE);
        }

        TEST_F(Sdl3RendererTest, DrawRect_LeavesWhatIsUnderneathShowing)
        {
            constexpr Color kUnder{
                .red = 200, .green = 0, .blue = 0, .alpha = 255};
            constexpr Color kOver{
                .red = 0, .green = 0, .blue = 255, .alpha = 48};

            auto &renderer = window->renderer();

            renderer.clear(kUnder);
            renderer.drawRect(
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = {.width = 8, .height = 8}},
                kOver);

            const auto pixel = pixelAt(Point{.x = 2, .y = 2});

            if (!pixel)
            {
                GTEST_SKIP() << "this driver reports no pixels back";
            }

            constexpr std::int32_t kBlendedRed = (200 * (255 - 48)) / 255;
            constexpr std::int32_t kBlendedBlue = (255 * 48) / 255;

            EXPECT_NEAR(pixel->red, kBlendedRed, 2);
            EXPECT_NEAR(pixel->blue, kBlendedBlue, 2);

            EXPECT_LT(pixel->blue, 255);
            EXPECT_GT(pixel->red, 0);
        }

        TEST_F(Sdl3RendererTest, DrawTexture_ScalesWithoutSmoothing)
        {
            constexpr Color kLeft{
                .red = 255, .green = 0, .blue = 0, .alpha = 255};
            constexpr Color kRight{
                .red = 0, .green = 0, .blue = 255, .alpha = 255};

            const Bitmap twoTexels{
                .size = {.width = 2, .height = 1},
                .pixels = {
                    kLeft.red,
                    kLeft.green,
                    kLeft.blue,
                    kLeft.alpha,
                    kRight.red,
                    kRight.green,
                    kRight.blue,
                    kRight.alpha}};

            auto &renderer = window->renderer();

            renderer.clear(Color{.alpha = 255});

            const auto texture = renderer.createTexture(twoTexels);

            renderer.drawTexture(
                *texture,
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = {.width = 2, .height = 1}},
                Rect{
                    .origin = {.x = 0, .y = 0},
                    .size = {.width = 32, .height = 8}},
                Color{
                    .red = 255, .green = 255, .blue = 255, .alpha = 255});

            const auto nearSeam = pixelAt(Point{.x = 15, .y = 4});
            const auto pastSeam = pixelAt(Point{.x = 16, .y = 4});

            if (!nearSeam || !pastSeam)
            {
                GTEST_SKIP() << "this driver reports no pixels back";
            }

            EXPECT_EQ(nearSeam->red, kLeft.red);
            EXPECT_EQ(nearSeam->blue, kLeft.blue);
            EXPECT_EQ(pastSeam->red, kRight.red);
            EXPECT_EQ(pastSeam->blue, kRight.blue);
        }
    }

}
