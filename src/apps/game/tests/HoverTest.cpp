#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/event/mocks/MockEventSink.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/PointerHint.hpp>
#include <antwika/input/PointerHintChannel.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/replay/ReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "TestTranslator.hpp"
#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/Game.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/GridScene.hpp"
#include "antwika/game/Hover.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/RenderSystem.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/WorldMap.hpp"
#include "antwika/game/WorldMapScene.hpp"
#include "antwika/game/WorldMapState.hpp"

using antwika::game::tests::kTranslator;

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::event::mocks::MockEventSink;
using antwika::game::AppMode;
using antwika::game::AppModeState;
using antwika::game::BuildingKind;
using antwika::game::BuildingSprite;
using antwika::game::Camera;
using antwika::game::Cell;
using antwika::game::cellCentre;
using antwika::game::GameSummary;
using antwika::game::GridExtent;
using antwika::game::GridScene;
using antwika::game::HoverReadout;
using antwika::game::hoverFor;
using antwika::game::HousingLevel;
using antwika::game::kUiCanvas;
using antwika::game::MainMenuScene;
using antwika::game::PathIndex;
using antwika::game::RenderSetup;
using antwika::game::RenderSystem;
using antwika::game::SaveLoadScene;
using antwika::game::SceneSnapshot;
using antwika::game::WalkerKind;
using antwika::game::WalkerSprite;
using antwika::game::WorldMapScene;
using antwika::game::WorldMapState;
using antwika::gfx::Size;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::gfx::mocks::MockWindow;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerHint;
using antwika::input::Position;
using antwika::log::mocks::MockLogger;
using antwika::replay::ReplaySource;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace
{
    constexpr GridExtent kExtent{.width = 16, .height = 16};
    constexpr antwika::time::Tick kMaxTicks = 40;
    const Camera kCamera{antwika::gfx::Point{.x = 300, .y = 40}, 3};

    [[nodiscard]] SceneSnapshot sceneOf(
        std::vector<BuildingSprite> buildings,
        std::vector<WalkerSprite> walkers = {})
    {
        return SceneSnapshot{
            .camera = kCamera,
            .extent = kExtent,
            .paths = {},
            .walkers = std::move(walkers),
            .buildings = std::move(buildings),
            .plan = {},
            .ghost = {},
            .hover = {}};
    }

    [[nodiscard]] PointerHint pointingAt(
        Cell cell, const Camera &camera = kCamera)
    {
        const auto centre = cellCentre(cell, camera);

        return PointerHint{
            .position = Position{.x = centre.x, .y = centre.y}};
    }

    // What a pointer over one cell of a scene is told, in words.
    [[nodiscard]] std::vector<std::string> saidOver(
        Cell cell, const BuildingSprite &building)
    {
        const auto panel = antwika::game::readoutPanel(
            hoverFor(
                pointingAt(cell), kCamera, sceneOf({building}), false),
            kUiCanvas,
            kTranslator);

        std::vector<std::string> said;
        for (const auto &line : panel.lines)
        {
            said.push_back(line.text);
        }

        return said;
    }

    [[nodiscard]] bool says(
        const std::vector<std::string> &said, std::string_view text)
    {
        return std::ranges::find(said, text) != said.end();
    }
} // namespace

TEST(HoverTest, HoverFor_ReportsNothingBeforeThePointerHasBeenSeen)
{
    const auto scene = sceneOf(
        {BuildingSprite{
            .at = Cell{.x = 2, .y = 2}, .kind = BuildingKind::House}});

    EXPECT_EQ(
        hoverFor(std::nullopt, kCamera, scene, false), HoverReadout{});
}

// What the bar covers, it covers from the readout too.
TEST(HoverTest, HoverFor_ReportsNothingUnderTheToolbar)
{
    const Cell where{.x = 2, .y = 2};
    const auto scene = sceneOf(
        {BuildingSprite{.at = where, .kind = BuildingKind::House}});

    EXPECT_EQ(
        hoverFor(pointingAt(where), kCamera, scene, true),
        HoverReadout{});
}

TEST(HoverTest, HoverFor_ReportsNothingOverBareGround)
{
    const auto scene = sceneOf(
        {BuildingSprite{
            .at = Cell{.x = 2, .y = 2}, .kind = BuildingKind::House}});

    const auto readout =
        hoverFor(pointingAt(Cell{.x = 9, .y = 9}), kCamera, scene, false);

    EXPECT_FALSE(readout.building.has_value());
    EXPECT_FALSE(readout.walker.has_value());
}

TEST(HoverTest, HoverFor_ReportsTheBuildingUnderThePointer)
{
    const Cell where{.x = 2, .y = 2};
    const BuildingSprite building{
        .at = where, .kind = BuildingKind::House, .stock = {40, 60}};

    const auto readout = hoverFor(
        pointingAt(where), kCamera, sceneOf({building}), false);

    ASSERT_TRUE(readout.building.has_value());
    EXPECT_EQ(*readout.building, building);
    EXPECT_FALSE(readout.walker.has_value());

    // Pinned where the pointer is, so the panel follows it.
    const auto centre = cellCentre(where, kCamera);
    EXPECT_EQ(readout.anchor, centre);
}

// Hovering a house says how many live there, out of what it holds.
TEST(HoverTest, AHoveredHouseSaysHowManyPeopleLiveInIt)
{
    const Cell where{.x = 2, .y = 2};
    const auto said = saidOver(
        where,
        BuildingSprite{
            .at = where,
            .kind = BuildingKind::House,
            .level = HousingLevel::Shack,
            .population = 6});

    EXPECT_TRUE(says(said, "house"));
    EXPECT_TRUE(says(said, "people 6/10"));
}

// And hovering something nobody lives in says nothing of the kind.
TEST(HoverTest, AHoveredWellSaysNothingAboutWhoLivesThere)
{
    const Cell where{.x = 2, .y = 2};
    const auto said = saidOver(
        where,
        BuildingSprite{
            .at = where,
            .kind = BuildingKind::Well,
            .population = 6});

    EXPECT_EQ(said, (std::vector<std::string>{"well"}));
}

// A building covers a block of cells rather than one.
// So hovering its far corner is hovering it.
TEST(HoverTest, HoverFor_ReportsABuildingFromAnyCellOfItsBlock)
{
    const Cell origin{.x = 2, .y = 2};
    const BuildingSprite building{
        .at = origin, .kind = BuildingKind::Storage};

    const auto footprint = antwika::game::footprintOf(building.kind);
    ASSERT_EQ(footprint.width, 3);

    for (std::int32_t dy = 0; dy < footprint.height; ++dy)
    {
        for (std::int32_t dx = 0; dx < footprint.width; ++dx)
        {
            const Cell cell{.x = origin.x + dx, .y = origin.y + dy};
            const auto readout = hoverFor(
                pointingAt(cell), kCamera, sceneOf({building}), false);

            ASSERT_TRUE(readout.building.has_value())
                << dx << "," << dy;
            EXPECT_EQ(readout.building->at, origin);
        }
    }

    // And the cell past its corner belongs to nobody.
    const auto beyond = hoverFor(
        pointingAt(Cell{.x = origin.x + footprint.width, .y = origin.y}),
        kCamera,
        sceneOf({building}),
        false);

    EXPECT_FALSE(beyond.building.has_value());
}

TEST(HoverTest, HoverFor_ReportsTheWalkerUnderThePointer)
{
    const Cell where{.x = 4, .y = 1};
    const WalkerSprite walker{
        .at = where, .kind = WalkerKind::MarketSeller, .carried = 70};

    const auto readout =
        hoverFor(pointingAt(where), kCamera, sceneOf({}, {walker}), false);

    ASSERT_TRUE(readout.walker.has_value());
    EXPECT_EQ(*readout.walker, walker);
    EXPECT_FALSE(readout.building.has_value());
}

// A walker is drawn over a building, so it wins the pointer too.
TEST(HoverTest, HoverFor_PrefersTheWalkerDrawnOverABuilding)
{
    const Cell where{.x = 3, .y = 3};

    const auto readout = hoverFor(
        pointingAt(where),
        kCamera,
        sceneOf(
            {BuildingSprite{.at = where, .kind = BuildingKind::House}},
            {WalkerSprite{.at = where}}),
        false);

    EXPECT_TRUE(readout.walker.has_value());
    EXPECT_FALSE(readout.building.has_value());
}

TEST(HoverTest, ReadoutEqualityComparesEveryField)
{
    const Cell where{.x = 2, .y = 2};
    const auto base = hoverFor(
        pointingAt(where),
        kCamera,
        sceneOf({BuildingSprite{.at = where, .kind = BuildingKind::House}}),
        false);

    EXPECT_EQ(base, base);

    auto moved = base;
    moved.anchor.x += 1;
    EXPECT_NE(base, moved);

    auto emptied = base;
    emptied.building.reset();
    EXPECT_NE(base, emptied);

    auto peopled = base;
    peopled.walker = WalkerSprite{};
    EXPECT_NE(base, peopled);
}

// The claim the whole channel rests on, asserted rather than assumed.
// One recorded stream, run with and without a pointer over the grid.
// Both have to reach the very same GameSummary.
// And the run with a pointer has to have drawn a readout.
// Otherwise the two would agree for the wrong reason.
namespace
{
    struct HintedRun
    {
        GameSummary summary;
        std::size_t texts = 0;
    };

    [[nodiscard]] std::vector<TickEvent> hoveredSession()
    {
        const InputEventCodec codec;
        std::vector<TickEvent> events;

        const auto pressAt = [&codec](Cell cell, MouseButton button)
        {
            const auto point = cellCentre(cell, Camera());

            return codec.encode(
                PointerButtonPressed{
                    .button = button,
                    .position = Position{.x = point.x, .y = point.y}});
        };

        for (std::int32_t x = 1; x <= 5; ++x)
        {
            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = pressAt(
                        Cell{.x = x, .y = 2}, MouseButton::Left)});
        }

        // The button comes back up where it went down last.
        // A road drag holds the run still until it does -- see RoadDrag.
        {
            const auto point = cellCentre(Cell{.x = 5, .y = 2}, Camera());

            events.push_back(
                TickEvent{
                    .tick = 0,
                    .event = codec.encode(
                        antwika::input::PointerButtonReleased{
                            .button = MouseButton::Left,
                            .position = Position{
                                .x = point.x, .y = point.y}})});
        }

        events.push_back(
            TickEvent{
                .tick = 1,
                .event =
                    pressAt(Cell{.x = 1, .y = 2}, MouseButton::Right)});

        events.push_back(
            TickEvent{
                .tick = 8,
                .event = Event{.name = antwika::engine::events::kStop}});

        return events;
    }

    [[nodiscard]] HintedRun runWatched(
        antwika::simulation::ITickEventSource &source,
        const std::optional<PointerHint> &pointer)
    {
        NiceMock<MockLogger> logger;
        NiceMock<MockEventSink> eventSink;
        const InputEventCodec codec;
        Camera camera;
        PathIndex paths;
        antwika::game::BuildingIndex built;
        AppModeState mode{AppMode::CityMap};
        antwika::game::PauseState pause;

        std::size_t texts = 0;

        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        NiceMock<MockTexture> atlas;
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));
        ON_CALL(window, size()).WillByDefault(Return(kUiCanvas));
        ON_CALL(renderer, drawText(_, _, _, _))
            .WillByDefault([&texts](auto &&...) { ++texts; });

        const GridScene scene{kTranslator};
        const MainMenuScene menuScene{kTranslator};
        const SaveLoadScene saveScene{kTranslator};
        const WorldMapScene worldScene;
        antwika::game::UiOverlay overlay{kUiCanvas};
        antwika::game::UiOverlay menuOverlay{kUiCanvas};
        antwika::game::UiOverlay saveOverlay{kUiCanvas};
        antwika::input::PointerHintChannel hint;
        WorldMapState cities{
            antwika::game::generateWorldMap(
                antwika::game::WorldMapConfig{
                    .width = 6, .height = 6, .seed = 1})};

        if (pointer.has_value())
        {
            hint.publish(*pointer);
        }

        RenderSystem renderSystem(
            RenderSetup{
                .window = window,
                .mode = mode,
                .canvas = kUiCanvas,
                .scene = scene,
                .atlas = atlas,
                .paths = paths,
                .built = built,
                .camera = camera,
                .extent = kExtent,
                .pause = pause,
                .overlay = overlay,
                .hint = hint,
                .menuScene = menuScene,
                .menuOverlay = menuOverlay,
                .saveScene = saveScene,
                .saveOverlay = saveOverlay,
                .worldScene = worldScene,
                .cities = cities});

        auto summary = antwika::game::bootstrap(
            antwika::game::GameConfig{
                .logger = logger,
                .eventSink = eventSink,
                .inputSource = source,
                .codec = codec,
                .extent = kExtent,
                .camera = camera,
                .paths = paths,
                .built = built,
                .mode = mode,
                .pause = pause,
                .observers = {renderSystem},
                .maxTicks = kMaxTicks});

        return HintedRun{.summary = std::move(summary), .texts = texts};
    }
} // namespace

TEST(HoverTest, AHoverChangesNothingAReplayReproduces)
{
    auto script = hoveredSession();
    ReplaySource blindSource(script);
    const auto blind = runWatched(blindSource, std::nullopt);

    auto watchedScript = hoveredSession();
    ReplaySource watchedSource(watchedScript);
    const auto watched = runWatched(
        watchedSource, pointingAt(Cell{.x = 3, .y = 2}, Camera()));

    // Neither run may have simply sat still.
    EXPECT_EQ(blind.summary.paths.size(), 5U);
    EXPECT_EQ(blind.summary.walkers.size(), 1U);

    // The pointer drew a readout, and reached nothing else at all.
    EXPECT_GT(watched.texts, 0U);
    EXPECT_EQ(blind.texts, 0U);
    EXPECT_EQ(watched.summary, blind.summary);
}
