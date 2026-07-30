// The shared backend contract lives in RaylibBackendConformanceTest.cpp.
// Only what needs raylib itself to set up is left here.
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <raylib.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "RaylibBackend.hpp"

// No using-declaration for Color: raylib declares one globally too.
using antwika::gfx::Resized;
using antwika::gfx::WindowDesc;
using antwika::gfx::raylib::RaylibBackend;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kPollLimit = 1000;

    WindowDesc resizableDesc()
    {
        return WindowDesc{
            .title = "Antwika raylib",
            .size = {.width = 320, .height = 240},
            .resizable = true};
    }

    // raylib opens its drawing bracket on the first drawing call.
    // Only closing it makes raylib look at the window system again.
    // So a frame has to draw something to count as one.
    void drawAFrame(antwika::gfx::IRenderer &renderer)
    {
        renderer.clear(antwika::gfx::Color{.red = 8, .green = 8, .blue = 8});
        renderer.present();
    }
} // namespace

TEST(RaylibBackendTest, PollEvent_DrainsAfterTheWindowIsResized)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(resizableDesc());
    auto &renderer = window->renderer();

    drawAFrame(renderer);

    SetWindowSize(500, 400);

    // This is the frame on which the new size arrives.
    drawAFrame(renderer);

    std::uint32_t polls = 0;
    bool sawResize = false;

    while (const auto event = backend.pollEvent())
    {
        sawResize = sawResize ||
                    std::holds_alternative<Resized>(event->payload);

        ++polls;

        // raylib reports live state, not a queue.
        // Its resized flag stays set until the next present.
        // Reporting straight off that flag makes this loop never end.
        ASSERT_LT(polls, kPollLimit)
            << "pollEvent never reported an empty queue after a resize";
    }

    EXPECT_TRUE(sawResize) << "the resize was never reported at all";
}

TEST(RaylibBackendTest, PollEvent_ReportsANewSizeOnlyOnce)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(resizableDesc());
    auto &renderer = window->renderer();

    drawAFrame(renderer);

    SetWindowSize(500, 400);
    drawAFrame(renderer);

    std::uint32_t resizes = 0;

    while (const auto event = backend.pollEvent())
    {
        if (const auto *resized = std::get_if<Resized>(&event->payload))
        {
            ++resizes;

            EXPECT_EQ(resized->size, window->size());
        }

        if (resizes > kPollLimit)
        {
            break;
        }
    }

    EXPECT_EQ(resizes, 1u);
}
