#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/gfx/BitmapRenderer.hpp"
#include "antwika/gfx/Bitmap.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/GfxError.hpp"
#include "antwika/gfx/Point.hpp"
#include "antwika/gfx/Rect.hpp"
#include "antwika/gfx/Size.hpp"

namespace
{
    using antwika::gfx::Bitmap;
    using antwika::gfx::BitmapRenderer;
    using antwika::gfx::Color;
    using antwika::gfx::GfxError;
    using antwika::gfx::kBytesPerPixel;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockTexture;
    using antwika::log::Level;
    using antwika::log::mocks::MockLogger;
    using ::testing::NiceMock;

    constexpr Color kRed{.red = 255, .green = 0, .blue = 0};

    constexpr Color kBlue{.red = 0, .green = 0, .blue = 255};

    class BitmapRendererTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] Color at(std::int32_t x, std::int32_t y) const
        {
            const auto &page = renderer.page();
            const auto slot =
                (static_cast<std::size_t>(y) * page.size.width + x)
                * kBytesPerPixel;

            return Color{
                .red = page.pixels[slot],
                .green = page.pixels[slot + 1],
                .blue = page.pixels[slot + 2],
                .alpha = page.pixels[slot + 3]};
        }

        [[nodiscard]] static Bitmap chequer()
        {
            Bitmap image;
            image.size = Size{.width = 2, .height = 2};
            image.pixels = std::vector<std::uint8_t>{
                255, 0,   0,   255,
                0,   255, 0,   255,
                0,   0,   255, 255,
                255, 255, 255, 0};

            return image;
        }

        NiceMock<MockLogger> logger;
        BitmapRenderer renderer{logger, Size{.width = 4, .height = 3}};
    };
}

TEST_F(BitmapRendererTest, Page_OpensOpaqueAndBlack)
{
    EXPECT_EQ(renderer.page().size, (Size{.width = 4, .height = 3}));
    EXPECT_TRUE(renderer.page().isComplete());

    for (std::int32_t y = 0; y < 3; ++y)
    {
        for (std::int32_t x = 0; x < 4; ++x)
        {
            EXPECT_EQ(at(x, y), (Color{.red = 0, .green = 0, .blue = 0}));
        }
    }
}

TEST_F(BitmapRendererTest, Constructor_RefusesAPageWithNoWidth)
{
    NiceMock<MockLogger> other;

    EXPECT_THROW(
        BitmapRenderer(other, Size{.width = 0, .height = 3}), GfxError);
}

TEST_F(BitmapRendererTest, Constructor_RefusesAPageWithNoHeight)
{
    NiceMock<MockLogger> other;

    EXPECT_THROW(
        BitmapRenderer(other, Size{.width = 4, .height = 0}), GfxError);
}

TEST_F(BitmapRendererTest, Clear_PaintsEveryPixelOpaque)
{
    renderer.clear(Color{.red = 10, .green = 20, .blue = 30, .alpha = 0});

    EXPECT_EQ(
        at(0, 0), (Color{.red = 10, .green = 20, .blue = 30}));
    EXPECT_EQ(
        at(3, 2), (Color{.red = 10, .green = 20, .blue = 30}));
}

TEST_F(BitmapRendererTest, DrawRect_FillsJustTheRectangle)
{
    renderer.drawRect(
        Rect{.origin = {.x = 1, .y = 1}, .size = {.width = 2, .height = 1}},
        kRed);

    EXPECT_EQ(at(1, 1), kRed);
    EXPECT_EQ(at(2, 1), kRed);
    EXPECT_EQ(at(0, 1), (Color{}));
    EXPECT_EQ(at(3, 1), (Color{}));
    EXPECT_EQ(at(1, 0), (Color{}));
    EXPECT_EQ(at(1, 2), (Color{}));
}

TEST_F(BitmapRendererTest, DrawRect_BlendsWhatIsAlreadyThere)
{
    renderer.clear(kRed);
    renderer.drawRect(
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Color{.red = 0, .green = 0, .blue = 255, .alpha = 128});

    const auto blended = at(0, 0);

    EXPECT_NEAR(blended.red, 127, 2);
    EXPECT_NEAR(blended.blue, 128, 2);
    EXPECT_EQ(blended.alpha, 255);
}

TEST_F(BitmapRendererTest, DrawRect_KeepsTheEdgesOfThePage)
{
    renderer.drawRect(
        Rect{
            .origin = {.x = -2, .y = -2},
            .size = {.width = 3, .height = 3}},
        kRed);

    renderer.drawRect(
        Rect{.origin = {.x = 3, .y = 2}, .size = {.width = 8, .height = 8}},
        kBlue);

    EXPECT_EQ(at(0, 0), kRed);
    EXPECT_EQ(at(1, 0), (Color{}));
    EXPECT_EQ(at(3, 2), kBlue);
}

TEST_F(BitmapRendererTest, DrawRect_DrawsNothingForAnEmptyRectangle)
{
    renderer.drawRect(
        Rect{.origin = {.x = 1, .y = 1}, .size = {.width = 0, .height = 0}},
        kRed);

    EXPECT_EQ(at(1, 1), (Color{}));
}

TEST_F(BitmapRendererTest, DrawLine_WalksFromEndToEnd)
{
    renderer.drawLine(Point{.x = 0, .y = 0}, Point{.x = 3, .y = 2}, kRed);

    EXPECT_EQ(at(0, 0), kRed);
    EXPECT_EQ(at(3, 2), kRed);
    EXPECT_EQ(at(0, 2), (Color{}));
}

TEST_F(BitmapRendererTest, DrawLine_WalksBackwardsAndStraightUp)
{
    renderer.drawLine(Point{.x = 2, .y = 2}, Point{.x = 2, .y = 0}, kBlue);

    EXPECT_EQ(at(2, 0), kBlue);
    EXPECT_EQ(at(2, 1), kBlue);
    EXPECT_EQ(at(2, 2), kBlue);
}

TEST_F(BitmapRendererTest, DrawLine_KeepsWhatFallsOffThePage)
{
    renderer.drawLine(
        Point{.x = -3, .y = 1}, Point{.x = 1, .y = 1}, kRed);

    EXPECT_EQ(at(0, 1), kRed);
    EXPECT_EQ(at(1, 1), kRed);
    EXPECT_EQ(at(2, 1), (Color{}));
}

TEST_F(BitmapRendererTest, DrawText_InksTheGlyphAndNothingElse)
{
    BitmapRenderer wide(logger, Size{.width = 32, .height = 16});

    wide.drawText(Point{.x = 0, .y = 0}, "1", 1, kRed);

    const auto &page = wide.page();
    std::size_t inked = 0;

    for (std::size_t slot = 0; slot < page.pixels.size();
         slot += kBytesPerPixel)
    {
        if (page.pixels[slot] > 0)
        {
            ++inked;
        }
    }

    EXPECT_GT(inked, 0U);
    EXPECT_LT(inked, 32U * 16U);
}

TEST_F(BitmapRendererTest, DrawText_KeepsWhatFallsOffThePage)
{
    renderer.drawText(Point{.x = -40, .y = 0}, "100", 1, kRed);
    renderer.drawText(Point{.x = 1, .y = -40}, "100", 1, kRed);

    EXPECT_EQ(at(0, 0), (Color{}));
    EXPECT_EQ(at(1, 0), (Color{}));
}

TEST_F(BitmapRendererTest, DrawText_KeepsWhatRunsPastTheFarCorner)
{
    renderer.drawText(Point{.x = 3, .y = 2}, "100", 1, kRed);

    EXPECT_EQ(at(0, 0), (Color{}));
    EXPECT_EQ(at(0, 2), (Color{}));
}

TEST_F(BitmapRendererTest, DrawLine_KeepsWhatRunsPastTheFarCorner)
{
    renderer.drawLine(Point{.x = 1, .y = 1}, Point{.x = 9, .y = 9}, kBlue);

    EXPECT_EQ(at(1, 1), kBlue);
    EXPECT_EQ(at(0, 0), (Color{}));
}

TEST_F(BitmapRendererTest, CreateTexture_RefusesAnIncompleteBitmap)
{
    Bitmap broken;
    broken.size = Size{.width = 4, .height = 4};

    EXPECT_THROW((void)renderer.createTexture(broken), GfxError);
}

TEST_F(BitmapRendererTest, DrawTexture_BlitsThePixelsItWasGiven)
{
    const auto texture = renderer.createTexture(chequer());

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 2, .height = 2}},
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 2, .height = 2}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_EQ(at(0, 0), (Color{.red = 255}));
    EXPECT_EQ(at(1, 0), (Color{.green = 255}));
    EXPECT_EQ(at(0, 1), (Color{.blue = 255}));
    EXPECT_EQ(at(1, 1), (Color{}));
}

TEST_F(BitmapRendererTest, DrawTexture_StretchesTheSourceOverTheDestination)
{
    const auto texture = renderer.createTexture(chequer());

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 4, .height = 3}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_EQ(at(0, 0), (Color{.red = 255}));
    EXPECT_EQ(at(3, 2), (Color{.red = 255}));
}

TEST_F(BitmapRendererTest, DrawTexture_TintsWhatItBlits)
{
    const auto texture = renderer.createTexture(chequer());

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Color{.red = 128, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_NEAR(at(0, 0).red, 128, 2);
}

TEST_F(BitmapRendererTest, DrawTexture_KeepsWhatHangsOffThePage)
{
    const auto texture = renderer.createTexture(chequer());

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 2, .height = 2}},
        Rect{
            .origin = {.x = -1, .y = -1},
            .size = {.width = 2, .height = 2}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 2, .height = 2}},
        Rect{.origin = {.x = 3, .y = 2}, .size = {.width = 2, .height = 2}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_EQ(at(0, 0), (Color{}));
    EXPECT_EQ(at(3, 2), (Color{.red = 255}));
}

TEST_F(BitmapRendererTest, DrawTexture_DeclinesATextureFromAnotherRenderer)
{
    MockLogger talkative;
    BitmapRenderer watched(talkative, Size{.width = 2, .height = 2});
    NiceMock<MockTexture> foreign;

    EXPECT_CALL(
        talkative,
        log(Level::Warning,
            "gfx.bitmap: declined a texture from another renderer"));

    watched.drawTexture(
        foreign,
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 1, .height = 1}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_EQ(watched.page().pixels[0], 0);
}

TEST_F(BitmapRendererTest, DrawTexture_DeclinesABlitOffTheTexture)
{
    const auto texture = renderer.createTexture(chequer());

    renderer.drawTexture(
        *texture,
        Rect{.origin = {.x = 1, .y = 1}, .size = {.width = 4, .height = 4}},
        Rect{.origin = {.x = 0, .y = 0}, .size = {.width = 2, .height = 2}},
        Color{.red = 255, .green = 255, .blue = 255, .alpha = 255});

    EXPECT_EQ(at(0, 0), (Color{}));
}

TEST_F(BitmapRendererTest, Present_TracesThatThePageIsDone)
{
    MockLogger talkative;
    BitmapRenderer watched(talkative, Size{.width = 2, .height = 2});

    EXPECT_CALL(talkative, log(Level::Trace, "gfx.bitmap: present"));

    watched.present();
}

TEST_F(BitmapRendererTest, Renderer3d_IsNotSomethingItDraws)
{
    EXPECT_EQ(renderer.renderer3d(), nullptr);
}
