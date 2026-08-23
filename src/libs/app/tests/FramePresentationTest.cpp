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
#include <antwika/gfx/RectF.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>

#include "antwika/app/FramePresentation.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Color;
using antwika::gfx::IRenderer;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    class Overlay final
    {
    public:
        explicit Overlay(DrawList pictureList) : picture(std::move(pictureList))
        {
        }

        [[nodiscard]] const DrawList &getCommands() const noexcept
        {
            return picture;
        }

    private:
        DrawList picture;
    };

    using OptionalOverlay =
        std::optional<std::reference_wrapper<const Overlay>>;

    constexpr Size kCanvasSize{.width = 100, .height = 100};

    constexpr Rect kSceneRect{
        .originPoint = {.x = 0, .y = 0},
        .size = {.width = 10, .height = 10}};
    constexpr Color kSceneColor{.red = 1, .green = 2, .blue = 3};

    constexpr Rect kOverlayRect{
        .originPoint = {.x = 5, .y = 5},
        .size = {.width = 4, .height = 4}};
    constexpr Color kOverlayColor{.red = 9, .green = 8, .blue = 7};

    constexpr Color kSurroundColor{.red = 0, .green = 0, .blue = 0};

    [[nodiscard]] TickEvent getTickEvent()
    {
        return TickEvent{
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    [[nodiscard]] Overlay getOverlay()
    {
        return Overlay(DrawList{
            FillRect{.rect = kOverlayRect, .color = kOverlayColor}});
    }
}

TEST(FramePresentationTest, DrawsOn_IsFalseForAnythingButTheTick)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(true));

    const TickEvent event{.event = Event{.name = "game.something"}};

    EXPECT_FALSE(antwika::app::shouldDraw(event, window));
}

TEST(FramePresentationTest, DrawsOn_IsFalseWhileTheWindowIsShut)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(false));

    EXPECT_FALSE(antwika::app::shouldDraw(getTickEvent(), window));
}

TEST(FramePresentationTest, DrawsOn_IsTrueOnATickIntoAnOpenWindow)
{
    NiceMock<MockWindow> window;
    ON_CALL(window, isOpen()).WillByDefault(Return(true));

    EXPECT_TRUE(antwika::app::shouldDraw(getTickEvent(), window));
}

TEST(FramePresentationTest, PaintOver_PaintsEveryCommandTheOverlayHolds)
{
    MockRenderer renderer;
    const auto picture = getOverlay();

    EXPECT_CALL(renderer, drawRect(RectF{kOverlayRect}, kOverlayColor));

    antwika::app::paintOverlay(renderer, picture);
}

TEST(FramePresentationTest, PaintOver_PaintsAnOverlayThatIsMounted)
{
    MockRenderer renderer;
    const auto picture = getOverlay();
    const OptionalOverlay mountedOverlay{std::cref(picture)};

    EXPECT_CALL(renderer, drawRect(RectF{kOverlayRect}, kOverlayColor));

    antwika::app::paintOverlay(renderer, mountedOverlay);
}

TEST(FramePresentationTest, PaintOver_PaintsNothingWhenNoneIsMounted)
{
    MockRenderer renderer;

    EXPECT_CALL(renderer, drawRect).Times(0);

    antwika::app::paintOverlay(renderer, OptionalOverlay{});
}

TEST(FramePresentationTest, PresentFrame_DrawsTheFrameThenPresentsIt)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    const InSequence order;
    EXPECT_CALL(renderer, drawRect(RectF{kSceneRect}, kSceneColor));
    EXPECT_CALL(renderer, present());

    antwika::app::presentFrame(
        window,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}

TEST(FramePresentationTest, PresentFrame_PaintsTheOverlayLastOfAll)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

    const auto picture = getOverlay();

    const InSequence order;
    EXPECT_CALL(renderer, drawRect(RectF{kSceneRect}, kSceneColor));
    EXPECT_CALL(renderer, drawRect(RectF{kOverlayRect}, kOverlayColor));
    EXPECT_CALL(renderer, present());

    antwika::app::presentFrame(
        window,
        picture,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}

TEST(FramePresentationTest, PresentViewport_FillsTheLetterboxLastOfAll)
{
    NiceMock<MockWindow> window;
    MockRenderer renderer;
    ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
    ON_CALL(window, getSize())
        .WillByDefault(Return(Size{.width = 200, .height = 100}));

    const auto picture = getOverlay();

    const InSequence order;

    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 50, .y = 0},
                .size = {.width = 10, .height = 10}}},
            kSceneColor));
    EXPECT_CALL(
        renderer,
        drawRect(
            RectF{Rect{
                .originPoint = {.x = 55, .y = 5},
                .size = {.width = 4, .height = 4}}},
            kOverlayColor));

    EXPECT_CALL(renderer, drawRect(_, kSurroundColor)).Times(2);
    EXPECT_CALL(renderer, present());

    antwika::app::presentViewport(
        window,
        kCanvasSize,
        kSurroundColor,
        picture,
        [](IRenderer &target)
        {
            target.drawRect(kSceneRect, kSceneColor);
        });
}
