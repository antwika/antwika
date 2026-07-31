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

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapLayout.hpp"
#include "antwika/game/WorldMapSink.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace
{

    using antwika::event::Event;
    using antwika::event::TickEvent;
    using antwika::game::AppMode;
    using antwika::game::AppModeState;
    using antwika::game::Camera;
    using antwika::game::Cell;
    using antwika::game::generateWorldMap;
    using antwika::game::InputFold;
    using antwika::game::kWorldMapKey;
    using antwika::game::PathIndex;
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

        // The boundary the app applies a staged mode at.
        void tick()
        {
            const TickEvent wrapped{
                .tick = 0,
                .event = Event{.name = antwika::engine::events::kTick}};
            input.handle(wrapped);
            mode.handle(wrapped);
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

        // Open a city and let the staged mode land, as a run does.
        void openCity(std::size_t city)
        {
            leftPressOn(state.world().cityCell(city));
            tick();
        }

        InputEventCodec codec;
        InputFold input{codec};
        WorldMapState state{generateWorldMap({16, 12, 11})};
        AppModeState mode{AppMode::WorldMap};
        PathIndex paths;
        Camera camera;
        WorldMapSink sink{state, mode, paths, camera, input, kCanvas};
    };

    TEST_F(WorldMapSinkTest, ClickingACityOpensIt)
    {
        leftPressOn(state.world().cityCell(2));

        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(state.city(), 2U);

        // Staged, so the click that opened it is not the grid's.
        EXPECT_EQ(mode.mode(), AppMode::WorldMap);
        EXPECT_EQ(mode.next(), AppMode::CityMap);
    }

    TEST_F(WorldMapSinkTest, OpeningACitySwapsItsGridIn)
    {
        state.cityPaths(2).insert(Cell{5, 6});

        openCity(2);

        EXPECT_TRUE(paths.has(Cell{5, 6}));
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
                    EXPECT_EQ(mode.next(), AppMode::WorldMap);
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

        EXPECT_EQ(mode.next(), AppMode::WorldMap);
    }

    TEST_F(WorldMapSinkTest, OnlyTheLeftButtonSelects)
    {
        send(
            PointerButtonPressed{
                .button = MouseButton::Right,
                .position = pixelOn(state.world().cityCell(0))});

        EXPECT_EQ(mode.next(), AppMode::WorldMap);
    }

    TEST_F(WorldMapSinkTest, ACityMapSwallowsAFurtherClick)
    {
        openCity(0);
        leftPressOn(state.world().cityCell(3));

        EXPECT_EQ(state.city(), 0U);
        EXPECT_EQ(mode.next(), AppMode::CityMap);
    }

    TEST_F(WorldMapSinkTest, TheWorldMapKeyGoesBackAndKeepsTheGrid)
    {
        openCity(1);
        paths.insert(Cell{2, 2});
        send(KeyPressed{.key = kWorldMapKey});

        EXPECT_FALSE(state.cityOpen());
        EXPECT_EQ(mode.next(), AppMode::WorldMap);
        EXPECT_TRUE(state.cityPaths(1).has(Cell{2, 2}));
    }

    TEST_F(WorldMapSinkTest, AHeldWorldMapKeyRepeatIsIgnored)
    {
        openCity(1);
        send(KeyPressed{.key = kWorldMapKey, .repeat = true});

        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(mode.next(), AppMode::CityMap);
    }

    TEST_F(WorldMapSinkTest, AnotherKeyDoesNothing)
    {
        openCity(1);
        send(KeyPressed{.key = Key::Escape});

        EXPECT_TRUE(state.cityOpen());
    }

    TEST_F(WorldMapSinkTest, TheWorldMapKeyOffACityDoesNothing)
    {
        send(KeyPressed{.key = kWorldMapKey});

        EXPECT_TRUE(state.cityOpen());
        EXPECT_EQ(mode.next(), AppMode::WorldMap);
    }

    TEST_F(WorldMapSinkTest, NonInputEventsAndOtherEdgesAreIgnored)
    {
        tick();
        send(PointerMoved{.position = pixelOn(state.world().cityCell(0))});
        send(
            PointerButtonReleased{
                .button = MouseButton::Left,
                .position = pixelOn(state.world().cityCell(0))});

        EXPECT_EQ(mode.next(), AppMode::WorldMap);
    }

} // namespace
