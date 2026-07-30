#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
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
     *
     * configuredSize() is the exception, and the reason it exists: it is
     * a number the caller chose, so it is the one window size every
     * backend can be held to exactly.
     *
     * Nothing here asserts that a resizable window can actually be
     * resized either, for the same reason -- there is no display to drag
     * an edge on, and a backend with no window system at all (null)
     * honours the flag by having nothing ever act on it. What a backend
     * is held to is that it accepts the request and keeps its promises
     * about both sizes afterwards.
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
         * @brief demoDesc(), but asking to be resizable.
         *
         * Deliberately a different size from demoDesc(), so a test
         * opening one of each can tell the two windows apart by what
         * they were configured with.
         */
        [[nodiscard]] static WindowDesc resizableDesc()
        {
            return WindowDesc{
                .title = "Antwika conformance, resizable",
                .size = {.width = 320, .height = 240},
                .resizable = true};
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

    TYPED_TEST_P(
        GfxBackendConformance, ConfiguredSize_IsExactlyWhatWasAskedFor)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        // The one window size a backend may be held to exactly.
        // Everything an application lays out hangs off this number.
        EXPECT_EQ(window->configuredSize(), this->demoDesc().size);
    }

    TYPED_TEST_P(
        GfxBackendConformance, ConfiguredSize_IsUnchangedByClosingTheWindow)
    {
        const auto window = this->backend->createWindow(this->demoDesc());

        window->close();

        EXPECT_EQ(window->configuredSize(), this->demoDesc().size);
    }

    TYPED_TEST_P(GfxBackendConformance, ConfiguredSize_IsPerWindow)
    {
        if (this->backend->maxWindows() < 2)
        {
            GTEST_SKIP() << "backend allows only one window at a time";
        }

        const auto first = this->backend->createWindow(this->demoDesc());
        const auto second =
            this->backend->createWindow(this->resizableDesc());

        // A backend keeping this in one global would fail here.
        EXPECT_EQ(first->configuredSize(), this->demoDesc().size);
        EXPECT_EQ(second->configuredSize(), this->resizableDesc().size);
    }

    TYPED_TEST_P(GfxBackendConformance, CreateWindow_AcceptsAResizableWindow)
    {
        const auto window =
            this->backend->createWindow(this->resizableDesc());

        ASSERT_NE(window, nullptr);
        EXPECT_TRUE(window->isOpen());

        // Nothing headless can drag the edge of a window.
        // The null backend has no window system to honour the flag with.
        // What every backend must do is take the request.
        // And then go on answering for itself afterwards.
        EXPECT_EQ(window->configuredSize(), this->resizableDesc().size);
        EXPECT_GT(window->size().width, 0u);
        EXPECT_GT(window->size().height, 0u);
    }

    TYPED_TEST_P(GfxBackendConformance, Size_StaysNonZeroAfterClosing)
    {
        const auto window =
            this->backend->createWindow(this->resizableDesc());

        window->close();

        // A closed window latches the last size it saw.
        // Reporting zero would hand a final frame a degenerate canvas.
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
        ConfiguredSize_IsExactlyWhatWasAskedFor,
        ConfiguredSize_IsUnchangedByClosingTheWindow,
        ConfiguredSize_IsPerWindow,
        CreateWindow_AcceptsAResizableWindow,
        Size_StaysNonZeroAfterClosing,
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
        Texture_MayOutliveItsWindow,
        PollEvent_DrainsToAnEmptyQueue,
        PollEvent_DrainsAfterAFrameIsDrawn,
        Window_MayOutliveItsBackend);

} // namespace antwika::gfx::conformance
