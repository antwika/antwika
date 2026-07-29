#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Rect.hpp>
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
            renderer.present();
        });
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
        PollEvent_DrainsToAnEmptyQueue);

} // namespace antwika::gfx::conformance
