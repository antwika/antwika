#include "antwika/app/FramePresentation.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <functional>
#include <optional>
#include <utility>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Color;
using antwika::gfx::IRenderer;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    // A stand-in for whatever an application paints last.
    // Anything answering commands() satisfies app::Pictured.
    // Which is the point: nothing inherits anything to be painted.
    class Overlay final
    {
    public:
        explicit Overlay(DrawList picture) : picture(std::move(picture))
        {
        }

        [[nodiscard]] const DrawList &commands() const noexcept
        {
            return picture;
        }

    private:
        DrawList picture;
    };

    using OptionalOverlay =
        std::optional<std::reference_wrapper<const Overlay>>;

    constexpr Size kCanvas{.width = 100, .height = 100};

    constexpr Rect kSceneRect{
        .origin = {.x = 0, .y = 0},
        .size = {.width = 10, .height = 10}};
    constexpr Color kSceneColor{.red = 1, .green = 2, .blue = 3};

    constexpr Rect kOverlayRect{
        .origin = {.x = 5, .y = 5},
        .size = {.width = 4, .height = 4}};
    constexpr Color kOverlayColor{.red = 9, .green = 8, .blue = 7};

    constexpr Color kSurround{.red = 0, .green = 0, .blue = 0};

    [[nodiscard]] TickEvent tickEvent()
    {
        return TickEvent{
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] Overlay overlay()
    {
        return Overlay(DrawList{
            FillRect{.rect = kOverlayRect, .color = kOverlayColor}});
    }
} // namespace

TEST(FramePresentationTest, DrawsOn_IsFalseForAnythingButTheTick)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(true));

    const TickEvent event{.event = Event{.name = "game.something"}};

    EXPECT_FALSE(antwika::app::drawsOn(event, window));
}

TEST(FramePresentationTest, DrawsOn_IsFalseWhileTheWindowIsShut)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(false));

    EXPECT_FALSE(antwika::app::drawsOn(tickEvent(), window));
}

TEST(FramePresentationTest, DrawsOn_IsTrueOnATickIntoAnOpenWindow)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(true));

    EXPECT_TRUE(antwika::app::drawsOn(tickEvent(), window));
}

TEST(FramePresentationTest, PaintOver_PaintsEveryCommandTheOverlayHolds)
{
    MockRenderer renderer;
    const auto picture = overlay();

    EXPECT_CALL(renderer, drawRect(kOverlayRect, kOverlayColor));

    antwika::app::paintOver(renderer, picture);
}

TEST(FramePresentationTest, PaintOver_PaintsAnOverlayThatIsMounted)
{
    MockRenderer renderer;
    const auto picture = overlay();
    const OptionalOverlay mounted{std::cref(picture)};

    EXPECT_CALL(renderer, drawRect(kOverlayRect, kOverlayColor));

    antwika::app::paintOver(renderer, mounted);
}

TEST(FramePresentationTest, PaintOver_PaintsNothingWhenNoneIsMounted)
{
    MockRenderer renderer;

    EXPECT_CALL(renderer, drawRect).Times(0);

    antwika::app::paintOver(renderer, OptionalOverlay{});
}

TEST(FramePresentationTest, PresentFrame_DrawsTheFrameThenPresentsIt)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    const InSequence order;
    EXPECT_CALL(renderer, drawRect(kSceneRect, kSceneColor));
    EXPECT_CALL(renderer, present());

    antwika::app::presentFrame(
        window,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}

// The ordering rule this helper exists to own.
// The overlay goes on after the body, and the present after both.
TEST(FramePresentationTest, PresentFrame_PaintsTheOverlayLastOfAll)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    const auto picture = overlay();

    const InSequence order;
    EXPECT_CALL(renderer, drawRect(kSceneRect, kSceneColor));
    EXPECT_CALL(renderer, drawRect(kOverlayRect, kOverlayColor));
    EXPECT_CALL(renderer, present());

    antwika::app::presentFrame(
        window,
        picture,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}

// A window twice the canvas's width, and the canvas's own height.
// So the picture is unscaled and centred, and the bars are real.
TEST(FramePresentationTest, PresentViewport_FillsTheSurroundLastOfAll)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, size())
        .WillByDefault(Return(Size{.width = 200, .height = 100}));

    const auto picture = overlay();

    const InSequence order;

    // Offset by the bar down the left, and not scaled.
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 50, .y = 0},
                .size = {.width = 10, .height = 10}},
            kSceneColor));
    EXPECT_CALL(
        renderer,
        drawRect(
            Rect{
                .origin = {.x = 55, .y = 5},
                .size = {.width = 4, .height = 4}},
            kOverlayColor));

    // One bar down each side, painted after the picture.
    EXPECT_CALL(renderer, drawRect).Times(2);
    EXPECT_CALL(renderer, present());

    antwika::app::presentViewport(
        window,
        kCanvas,
        kSurround,
        picture,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}
