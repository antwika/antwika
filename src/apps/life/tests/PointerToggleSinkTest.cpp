#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/ecs/World.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/Events.hpp>
#include <antwika/input/InputError.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/life/PointerToggleSink.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/DragState.hpp"
#include "antwika/life/Events.hpp"
#include "antwika/life/Grid.hpp"

using antwika::ecs::World;
using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::Size;
using antwika::input::InputError;
using antwika::input::InputEvent;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::PointerScrolled;
using antwika::input::Position;
using antwika::life::Cell;
using antwika::life::DragState;
using antwika::life::Grid;
using antwika::life::PointerToggleSink;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{
    constexpr std::uint32_t kWidth = 4;
    constexpr std::uint32_t kHeight = 4;

    constexpr Size kCanvas{.width = 40, .height = 40};

    class PointerToggleSinkTest : public ::testing::Test
    {
    protected:
        PointerToggleSinkTest()
        {
            world.commit();
        }

        void feed(const InputEvent &edge)
        {
            sink.handle(
                TickEvent{.tick = tick, .event = codec.encode(edge)});
        }

        void press(std::int32_t x, std::int32_t y)
        {
            feed(PointerButtonPressed{
                .button = MouseButton::Left,
                .position = Position{.x = x, .y = y}});
        }

        void moveTo(std::int32_t x, std::int32_t y)
        {
            feed(PointerMoved{.position = Position{.x = x, .y = y}});
        }

        void release(std::int32_t x, std::int32_t y)
        {
            feed(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = Position{.x = x, .y = y}});
        }

        void tickPasses()
        {
            world.commit();
            sink.handle(TickEvent{
                .tick = tick,
                .event = Event{.name = antwika::engine::events::kTick}});
            ++tick;
        }

        [[nodiscard]] bool aliveAt(std::uint32_t x, std::uint32_t y)
        {
            world.commit();

            return world.get<Cell>(grid.entityAt(x, y)).alive;
        }

        [[nodiscard]] std::size_t aliveCount()
        {
            world.commit();

            std::size_t alive = 0;
            for (std::uint32_t y = 0; y < kHeight; ++y)
            {
                for (std::uint32_t x = 0; x < kWidth; ++x)
                {
                    if (world.get<Cell>(grid.entityAt(x, y)).alive)
                    {
                        ++alive;
                    }
                }
            }
            return alive;
        }

        NiceMock<MockLogger> logger;
        World world{logger};
        Grid grid{world, kWidth, kHeight};
        InputEventCodec codec;
        antwika::time::Tick tick = 0;
        DragState drag;
        PointerToggleSink sink{world, grid, codec, kCanvas, drag};
    };
}

TEST_F(PointerToggleSinkTest, Handle_TogglesTheCellUnderALeftPress)
{
    press(25, 34);

    EXPECT_TRUE(aliveAt(2, 3));
    EXPECT_EQ(aliveCount(), 1u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresAPressOutsideTheBoard)
{
    press(-1, 5);
    press(400, 5);

    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresAButtonOtherThanLeft)
{
    feed(PointerButtonPressed{
        .button = MouseButton::Right, .position = Position{.x = 5, .y = 5}});

    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresMovementWithNothingHeld)
{
    moveTo(5, 5);
    moveTo(15, 5);

    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_TogglesEveryCellADragCrosses)
{
    press(5, 5);
    moveTo(15, 5);
    moveTo(25, 5);

    EXPECT_TRUE(aliveAt(0, 0));
    EXPECT_TRUE(aliveAt(1, 0));
    EXPECT_TRUE(aliveAt(2, 0));
    EXPECT_EQ(aliveCount(), 3u);
}

TEST_F(PointerToggleSinkTest, Handle_TogglesTheCellsAFastDragJumped)
{
    press(5, 5);
    moveTo(35, 5);

    for (std::uint32_t x = 0; x < kWidth; ++x)
    {
        EXPECT_TRUE(aliveAt(x, 0)) << x;
    }

    EXPECT_EQ(aliveCount(), 4u);
}

TEST_F(PointerToggleSinkTest, Handle_DoesNotWalkAJumpWiderThanTheCanvas)
{
    press(5, 5);
    moveTo(2000000000, 5);
    moveTo(25, 5);

    EXPECT_TRUE(aliveAt(0, 0));
    EXPECT_TRUE(aliveAt(2, 0));
    EXPECT_EQ(aliveCount(), 2u);
}

TEST_F(PointerToggleSinkTest, Handle_TogglesACellOnlyOncePerDrag)
{
    press(5, 5);
    moveTo(6, 5);
    moveTo(7, 6);
    moveTo(5, 5);

    EXPECT_TRUE(aliveAt(0, 0));
    EXPECT_EQ(aliveCount(), 1u);
}

TEST_F(PointerToggleSinkTest, Handle_LeavesACellAloneWhenADragReturnsToIt)
{
    press(5, 5);
    moveTo(15, 5);
    moveTo(5, 5);

    EXPECT_TRUE(aliveAt(0, 0));
    EXPECT_TRUE(aliveAt(1, 0));
    EXPECT_EQ(aliveCount(), 2u);
}

TEST_F(PointerToggleSinkTest, Handle_StopsTogglingOnceTheButtonIsReleased)
{
    press(5, 5);
    release(5, 5);
    moveTo(15, 5);
    moveTo(25, 5);

    EXPECT_TRUE(aliveAt(0, 0));
    EXPECT_EQ(aliveCount(), 1u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresAReleaseOfAnotherButton)
{
    press(5, 5);
    feed(PointerButtonReleased{
        .button = MouseButton::Middle,
        .position = Position{.x = 5, .y = 5}});
    moveTo(15, 5);

    EXPECT_TRUE(aliveAt(1, 0));
    EXPECT_EQ(aliveCount(), 2u);
}

TEST_F(PointerToggleSinkTest, Handle_ReportsADragWhileTheButtonIsDown)
{
    EXPECT_FALSE(drag.inProgress());

    press(5, 5);
    EXPECT_TRUE(drag.inProgress());

    moveTo(15, 5);
    EXPECT_TRUE(drag.inProgress());

    release(15, 5);
    EXPECT_FALSE(drag.inProgress());
}

TEST_F(PointerToggleSinkTest, Handle_ReportsADragThatStartedOffTheBoard)
{
    press(-5, -5);

    EXPECT_TRUE(drag.inProgress());
    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_ReportsNoDragForAnotherButton)
{
    feed(PointerButtonPressed{
        .button = MouseButton::Right, .position = Position{.x = 5, .y = 5}});

    EXPECT_FALSE(drag.inProgress());
}

TEST_F(PointerToggleSinkTest, Handle_TogglesALiveCellBackToDead)
{
    press(5, 5);
    release(5, 5);
    tickPasses();

    ASSERT_TRUE(aliveAt(0, 0));

    press(5, 5);

    EXPECT_FALSE(aliveAt(0, 0));
}

TEST_F(PointerToggleSinkTest, Handle_TogglesTwiceForTwoDragsInOneTick)
{
    press(5, 5);
    release(5, 5);
    press(5, 5);

    EXPECT_FALSE(aliveAt(0, 0));
    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_ReadsTheBoardAfreshOnceATickHasPassed)
{
    press(5, 5);
    release(5, 5);
    tickPasses();

    world.set<Cell>(grid.entityAt(0, 0), Cell{.alive = false});
    world.commit();

    press(5, 5);

    EXPECT_TRUE(aliveAt(0, 0));
}

TEST_F(PointerToggleSinkTest, Handle_ClearsWhatADragVisitedWhenANewOneStarts)
{
    press(5, 5);
    moveTo(15, 5);
    release(15, 5);

    press(5, 5);
    moveTo(15, 5);

    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresInputItHasNoUseFor)
{
    press(5, 5);
    feed(PointerScrolled{.vertical = 3});

    EXPECT_EQ(aliveCount(), 1u);
}

TEST_F(PointerToggleSinkTest, Handle_IgnoresEventsThatAreNotInput)
{
    sink.handle(TickEvent{
        .tick = 0, .event = Event{.name = antwika::engine::events::kTick}});
    sink.handle(TickEvent{
        .tick = 0,
        .event = Event{
            .name = antwika::life::events::kToggleCell,
            .payload = R"({"x":1,"y":1})"}});

    EXPECT_EQ(aliveCount(), 0u);
}

TEST_F(PointerToggleSinkTest, Handle_ThrowsOnAMalformedInputPayload)
{
    EXPECT_THROW(
        sink.handle(TickEvent{
            .tick = 0,
            .event = Event{
                .name = antwika::input::events::kPointerDown,
                .payload = "{"}}),
        InputError);
}

TEST(PointerToggleSinkCanvasTest, Handle_IgnoresAClickOnACanvasTooSmallToDraw)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    Grid grid(world, kWidth, kHeight);
    world.commit();
    const InputEventCodec codec;

    DragState drag;
    PointerToggleSink sink(
        world, grid, codec, Size{.width = 2, .height = 2}, drag);

    sink.handle(TickEvent{
        .tick = 0,
        .event = codec.encode(PointerButtonPressed{
            .button = MouseButton::Left,
            .position = Position{.x = 1, .y = 1}})});

    world.commit();

    EXPECT_FALSE(world.get<Cell>(grid.entityAt(0, 0)).alive);
}
