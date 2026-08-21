#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "RaylibBackend.hpp"

using antwika::gfx::Bitmap;
using antwika::gfx::Color;
using antwika::gfx::kBytesPerPixel;
using antwika::gfx::Rect;
using antwika::gfx::WindowSpec;
using antwika::gfx::raylib::RaylibBackend;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kWidth = 64;

    constexpr std::uint32_t kHeight = 48;

    WindowSpec demoSpec()
    {
        return WindowSpec{
            .title = "Antwika raylib readback",
            .size = {.width = kWidth, .height = kHeight},
            .hidden = true};
    }

    Color pixelAt(
        const Bitmap &bitmap,
        const std::uint32_t x,
        const std::uint32_t y)
    {
        const auto byteIndex =
            ((static_cast<std::size_t>(y) * bitmap.size.width) + x)
            * kBytesPerPixel;

        return Color{
            .red = bitmap.pixels[byteIndex],
            .green = bitmap.pixels[byteIndex + 1],
            .blue = bitmap.pixels[byteIndex + 2],
            .alpha = bitmap.pixels[byteIndex + 3]};
    }
}

TEST(RaylibReadPixelsTest, ReadPixels_ComesBackTheSizeOfTheDrawable)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    renderer.clear(Color{.red = 8, .green = 16, .blue = 32});

    const Bitmap takenBitmap = renderer.readPixels();

    ASSERT_TRUE(takenBitmap.isValid());
    EXPECT_EQ(kWidth, takenBitmap.size.width);
    EXPECT_EQ(kHeight, takenBitmap.size.height);
}

TEST(RaylibReadPixelsTest, ReadPixels_SeesWhatWasJustDrawn)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    const Color backgroundColor{.red = 8, .green = 16, .blue = 32};
    const Color markColor{.red = 255, .green = 0, .blue = 0};

    renderer.clear(backgroundColor);
    renderer.drawRect(
        Rect{
            .originPoint = {.x = 4, .y = 4},
            .size = {.width = 8, .height = 8}},
        markColor);

    const Bitmap takenBitmap = renderer.readPixels();

    ASSERT_TRUE(takenBitmap.isValid());

    EXPECT_EQ(markColor, pixelAt(takenBitmap, 8, 8));
    EXPECT_EQ(backgroundColor, pixelAt(takenBitmap, 32, 32));
}

TEST(RaylibReadPixelsTest, ReadPixels_ComesBackEmptyOnceTheWindowCloses)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(demoSpec());
    auto &renderer = window->renderer();

    window->close();

    EXPECT_TRUE(renderer.readPixels().pixels.empty());
}
