#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>

#include "antwika/game/Cell.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapLayout.hpp"
#include "antwika/game/WorldMapSink.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace
{

    using antwika::event::Event;
    using antwika::event::TickEvent;
    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::InputFold;
    using antwika::game::kWorldMapKey;
    using antwika::game::MapView;
    using antwika::game::WorldMapSink;
    using antwika::game::WorldMapState;
    using antwika::game::worldTileRect;
    using antwika::gfx::Size;
    using antwika::input::InputEvent;
    using antwika::input::InputEventCodec;
    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;
    using antwika::input::Position;

    constexpr Size kCanvas{.width = 1024, .height = 640};

    class WorldMapSinkTest : public ::testing::Test
    {
    protected:
        // Through the fold first, as the app registers it.
        // The fold runs ahead of this sink.
        void send(const InputEvent &event)
        {
            const TickEvent wrapped{
                .tick = 0, .event = codec.encode(event)};
            input.handle(wrapped);
            sink.handle(wrapped);
        }

        void tick()
        {
            const TickEvent wrapped{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}};
            input.handle(wrapped);
            sink.handle(wrapped);
        }

        [[nodiscard]] Position pixelOn(Cell cell) const
        {
            const auto rect = worldTileRect(
                kCanvas,
                state.world().width,
                state.world().height,
                cell);
            return Position{
                .x = rect.origin.x + 1, .y = rect.origin.y + 1};
        }

        void leftPressOn(Cell cell)
        {
            send(
                PointerButtonPressed{
                    .button = MouseButton::Left,
                    .position = pixelOn(cell)});
        }

        InputEventCodec codec;
        InputFold input{codec};
        WorldMapState state{generateWorldMap({16, 12, 11})};
        WorldMapSink sink{state, input, kCanvas};
    };

    TEST_F(WorldMapSinkTest, ClickingACityOpensIt)
    {
        leftPressOn(state.world().cityCell(2));

        EXPECT_EQ(state.view(), MapView::City);
        EXPECT_EQ(state.openCity(), 2U);
    }

    TEST_F(WorldMapSinkTest, ClickingEmptyLandOpensNothing)
    {
        // A cell no city sits on, found by asking the map.
        for (std::uint32_t y = 0; y < state.world().height; ++y)
        {
            for (std::uint32_t x = 0; x < state.world().width; ++x)
            {
                const Cell cell{
                    static_cast<std::int32_t>(x),
                    static_cast<std::int32_t>(y)};
                if (state.world().cityAt(cell)
                    == antwika::game::kCityCount)
                {
                    leftPressOn(cell);
                    EXPECT_EQ(state.view(), MapView::World);
                    return;
                }
            }
        }
        FAIL() << "every cell held a city";
    }

    TEST_F(WorldMapSinkTest, ClickingOffTheMapOpensNothing)
    {
        send(
            PointerButtonPressed{
                .button = MouseButton::Left,
                .position = Position{.x = 0, .y = 0}});

        EXPECT_EQ(state.view(), MapView::World);
    }

    TEST_F(WorldMapSinkTest, OnlyTheLeftButtonSelects)
    {
        send(
            PointerButtonPressed{
                .button = MouseButton::Right,
                .position = pixelOn(state.world().cityCell(0))});

        EXPECT_EQ(state.view(), MapView::World);
    }

    TEST_F(WorldMapSinkTest, ACityMapSwallowsAFurtherClick)
    {
        leftPressOn(state.world().cityCell(0));
        leftPressOn(state.world().cityCell(3));

        EXPECT_EQ(state.openCity(), 0U);
    }

    TEST_F(WorldMapSinkTest, TheWorldMapKeyGoesBack)
    {
        leftPressOn(state.world().cityCell(1));
        send(KeyPressed{.key = kWorldMapKey});

        EXPECT_EQ(state.view(), MapView::World);
    }

    TEST_F(WorldMapSinkTest, AHeldWorldMapKeyRepeatIsIgnored)
    {
        leftPressOn(state.world().cityCell(1));
        send(KeyPressed{.key = kWorldMapKey, .repeat = true});

        EXPECT_EQ(state.view(), MapView::City);
    }

    TEST_F(WorldMapSinkTest, AnotherKeyDoesNothing)
    {
        leftPressOn(state.world().cityCell(1));
        send(KeyPressed{.key = Key::Escape});

        EXPECT_EQ(state.view(), MapView::City);
    }

    TEST_F(WorldMapSinkTest, NonInputEventsAndOtherEdgesAreIgnored)
    {
        tick();
        send(PointerMoved{.position = pixelOn(state.world().cityCell(0))});
        send(
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = pixelOn(state.world().cityCell(0))});

        EXPECT_EQ(state.view(), MapView::World);
    }

} // namespace
