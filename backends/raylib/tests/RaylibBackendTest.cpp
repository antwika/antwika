#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <raylib.h>

#include <cstdint>
#include <cstdlib>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowSpec.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "RaylibBackend.hpp"

using antwika::gfx::Resized;
using antwika::gfx::WindowSpec;
using antwika::gfx::raylib::RaylibBackend;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kPollLimit = 1000;

    WindowSpec getResizableSpec()
    {
        return WindowSpec{
            .title = "Antwika raylib",
            .size = {.width = 320, .height = 240},
            .resizable = true,
            .hidden = true};
    }

    void drawAFrame(antwika::gfx::IRenderer &renderer)
    {
        renderer.clear(antwika::gfx::Color{.red = 8, .green = 8, .blue = 8});
        renderer.present();
    }
}

TEST(RaylibBackendTest, PollEvent_DrainsAfterTheWindowIsResized)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(getResizableSpec());
    auto &renderer = window->renderer();

    drawAFrame(renderer);

    SetWindowSize(500, 400);

    drawAFrame(renderer);

    std::uint32_t polls = 0;
    bool sawResize = false;

    while (const auto event = backend.pollEvent())
    {
        sawResize = sawResize ||
                    std::holds_alternative<Resized>(event->payload);

        ++polls;

        ASSERT_LT(polls, kPollLimit);
    }

    EXPECT_TRUE(sawResize);
}

TEST(RaylibBackendTest, PollEvent_ReportsANewSizeOnlyOnce)
{
    NiceMock<MockLogger> logger;
    RaylibBackend backend(logger);

    const auto window = backend.createWindow(getResizableSpec());
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

            EXPECT_EQ(resized->size, window->getSize());
        }

        if (resizes > kPollLimit)
        {
            break;
        }
    }

    EXPECT_EQ(resizes, 1u);
}

TEST(RaylibBackendDeathTest, CreateWindow_FailsCleanlyWithNoDisplay)
{
    EXPECT_EXIT(
        {
            ::setenv("DISPLAY", ":9999", 1);

            NiceMock<MockLogger> logger;
            RaylibBackend backend(logger);

            try
            {
                (void)backend.createWindow(getResizableSpec());
            }
            catch (const antwika::gfx::GfxError &)
            {
                std::_Exit(42);
            }

            std::_Exit(0);
        },
        ::testing::ExitedWithCode(42),
        "");
}
