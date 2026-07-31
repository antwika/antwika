#include "Sdl3Renderer.hpp"

#include <SDL3/SDL.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <optional>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "Sdl3Backend.hpp"

namespace antwika::gfx::sdl3
{

    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    namespace
    {
        /**
         * @brief What a colour and a blend mode do to a real drawable.
         *
         * The conformance suite cannot ask this of any backend, because
         * IRenderer reports no pixel back and never will.
         * SDL does report its own, though, which is enough to hold this
         * backend to the one thing that suite has to leave open: that an
         * alpha below 255 blends rather than overwriting.
         *
         * Nothing here reaches through IRenderer.
         * The SDL renderer is found the way any SDL program would find
         * one, from the window id the backend already publishes.
         */
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

            /**
             * @brief Read one pixel back, through SDL rather than gfx.
             * @param at Which pixel to read.
             * @return Its colour, or nothing if this driver declined.
             */
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

        TEST_F(Sdl3RendererTest, DrawText_BlendsRatherThanOverwriting)
        {
            window->renderer().drawText(
                Point{.x = 1, .y = 1},
                "Antwika",
                1,
                Color{.green = 255, .alpha = 200});

            EXPECT_EQ(blendMode(), SDL_BLENDMODE_BLEND);
        }

        // A clear replaces the drawable area rather than drawing into it.
        // Blending here would mix each frame with the one before it.
        // It is also what raylib's glClear does, so the two agree.
        TEST_F(Sdl3RendererTest, Clear_ReplacesRatherThanBlending)
        {
            window->renderer().clear(Color{.red = 8, .green = 8, .blue = 8});

            EXPECT_EQ(blendMode(), SDL_BLENDMODE_NONE);
        }

        // The one test here that looks at a pixel.
        // A blend mode is evidence; this is the bug itself.
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

            // Straight alpha over an opaque destination.
            // Rounded to whichever neighbouring integer a driver picks.
            constexpr std::int32_t kBlendedRed = (200 * (255 - 48)) / 255;
            constexpr std::int32_t kBlendedBlue = (255 * 48) / 255;

            EXPECT_NEAR(pixel->red, kBlendedRed, 2);
            EXPECT_NEAR(pixel->blue, kBlendedBlue, 2);

            // The failure this is really about, said the other way.
            // Unblended, the fill lands as a flat blue over the red.
            EXPECT_LT(pixel->blue, 255);
            EXPECT_GT(pixel->red, 0);
        }
    } // namespace

} // namespace antwika::gfx::sdl3
