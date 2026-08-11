#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

#include <antwika/ecs/World.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>

#include "antwika/map_editor/Commands.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/EntityEditSystem.hpp"
#include "antwika/map_editor/MirrorSystem.hpp"
#include "antwika/map_editor/ValidationSystem.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace widgets = antwika::map_editor::widgets;

using antwika::ecs::World;
using antwika::geometry::GridCell;
using antwika::log::mocks::MockLogger;
using antwika::map_editor::CellRef;
using antwika::map_editor::EditorStore;
using antwika::map_editor::EntityEditSystem;
using antwika::map_editor::Marker;
using antwika::map_editor::MarkerKind;
using antwika::map_editor::MirrorSystem;
using antwika::map_editor::pinAll;
using antwika::map_editor::ValidationSystem;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Pickup;
using antwika::tilemap::TileMap;
using antwika::tilemap::Transition;
using ::testing::NiceMock;

namespace
{
    constexpr antwika::time::Tick kTick{};

    [[nodiscard]] GridCell cellAt(
        const std::uint32_t column, const std::uint32_t row)
    {
        return GridCell{.column = column, .row = row};
    }

    [[nodiscard]] EditorStore storeOf()
    {
        EditorStore store{
            .state = {.map = TileMap{MapHeader{}, 4, 4}}};
        pinAll(store.state);

        return store;
    }

    [[nodiscard]] std::size_t markerCount(World &world)
    {
        std::size_t count = 0;

        for (const auto entity : world.view<Marker, CellRef>())
        {
            static_cast<void>(entity);
            ++count;
        }

        return count;
    }
}

TEST(MirrorSystemTest, Update_SpawnsOneMarkerPerMapEntity)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper", .at = cellAt(1, 2)});
    store.state.map.addEntity(
        Transition{.id = "door", .at = cellAt(3, 0), .level = 2});
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    EXPECT_EQ(markerCount(world), 2U);
}

TEST(MirrorSystemTest, Update_CarriesTheKindCellAndLevel)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(
        Transition{.id = "door", .at = cellAt(3, 1), .level = 2});
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    for (const auto entity : world.view<Marker, CellRef>())
    {
        EXPECT_EQ(
            world.get<Marker>(entity).kind, MarkerKind::Transition);
        EXPECT_EQ(world.get<Marker>(entity).index, 0U);
        EXPECT_EQ(world.get<CellRef>(entity).column, 3U);
        EXPECT_EQ(world.get<CellRef>(entity).row, 1U);
        EXPECT_EQ(world.get<CellRef>(entity).level, 2);
    }
}

TEST(MirrorSystemTest, Update_SkipsRespawningAnUnchangedList)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();
    system.update(world, kTick);
    world.commit();

    EXPECT_EQ(markerCount(world), 1U);
}

TEST(MirrorSystemTest, Update_RespawnsAfterTheListChanges)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    store.state.map.addEntity(Npc{.id = "friend"});
    system.update(world, kTick);
    world.commit();

    EXPECT_EQ(markerCount(world), 2U);
}

TEST(MirrorSystemTest, Update_ClearsASelectionPastTheShrunkList)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    store.ui.selected = 5;
    store.ui.idField.text = "stale";
    store.state.map = TileMap{MapHeader{}, 4, 4};
    system.update(world, kTick);
    world.commit();

    EXPECT_FALSE(store.ui.selected.has_value());
    EXPECT_TRUE(store.ui.idField.text.empty());
}

TEST(MirrorSystemTest, Update_ReloadsTheBuffersOfAKeptSelection)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.selected = 0;
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    EXPECT_EQ(store.ui.idField.text, "keeper");
}

TEST(MirrorSystemTest, Update_LeavesTheBuffersAloneWhileAFieldHasFocus)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.selected = 0;
    store.ui.focus = widgets::kFieldId;
    store.ui.idField.text = "typing";
    MirrorSystem system{store};

    system.update(world, kTick);
    world.commit();

    EXPECT_EQ(store.ui.idField.text, "typing");
}

TEST(ValidationSystemTest, Update_RefreshesTheReport)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.overlayOn = true;
    store.state.reportStale = true;
    ValidationSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.state.report.has_value());
}

TEST(ValidationSystemTest, Update_CountsDownTheFailedGenerateBanner)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.generateFailedTicks = 2;
    ValidationSystem system{store};

    system.update(world, kTick);
    EXPECT_EQ(store.state.generateFailedTicks, 1U);

    system.update(world, kTick);
    EXPECT_EQ(store.state.generateFailedTicks, 0U);

    system.update(world, kTick);
    EXPECT_EQ(store.state.generateFailedTicks, 0U);
}

TEST(EntityEditSystemTest, Update_PlacesTheChosenKind)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.placeKind =
        static_cast<std::size_t>(MarkerKind::Npc);
    store.ui.acted.activated = widgets::kPlace;
    EntityEditSystem system{store};

    system.update(world, kTick);

    ASSERT_EQ(store.state.map.entities().size(), 1U);
    EXPECT_TRUE(
        std::holds_alternative<Npc>(store.state.map.entities()[0]));
}

TEST(EntityEditSystemTest, Update_WrapsAPlaceKindPastTheLastOne)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.placeKind = antwika::map_editor::kMarkerKindCount;
    store.ui.acted.activated = widgets::kPlace;
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(std::holds_alternative<Transition>(
        store.state.map.entities()[0]));
}

TEST(EntityEditSystemTest, Update_DeletesTheEntitiesAtTheHoveredCell)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.hovered = cellAt(1, 1);
    antwika::map_editor::placeNpc(store.state);
    store.ui.acted.activated = widgets::kDelete;
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.state.map.entities().empty());
}

TEST(EntityEditSystemTest, Update_ClearsTheActionItJustHandled)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.acted.activated = widgets::kPlace;
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.ui.acted.activated, antwika::ui::kNoWidget);
}

TEST(EntityEditSystemTest, Update_TypesIntoTheIdField)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId, .text = "keeper", .cursor = 6};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.ui.idField.text, "keeper");
    EXPECT_EQ(store.ui.idField.cursor, 6U);
}

TEST(EntityEditSystemTest, Update_TypesIntoEachEntityField)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EntityEditSystem system{store};

    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldTargetMap, .text = "next"};
    system.update(world, kTick);
    EXPECT_EQ(store.ui.targetMapField.text, "next");

    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldTargetEntry, .text = "back"};
    system.update(world, kTick);
    EXPECT_EQ(store.ui.targetEntryField.text, "back");

    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldTags, .text = "wet"};
    system.update(world, kTick);
    EXPECT_EQ(store.ui.tagsField.text, "wet");
}

TEST(EntityEditSystemTest, Update_TypesIntoTheCharacterNameField)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kCharName, .text = "hero", .cursor = 4};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.characters.nameField.text, "hero");
    EXPECT_EQ(store.characters.nameField.cursor, 4U);
}

TEST(EntityEditSystemTest, Update_TypesIntoTheSocketNameField)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kSocketName, .text = "rim", .cursor = 3};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.tilesets.socketNameField.text, "rim");
    EXPECT_EQ(store.tilesets.socketNameField.cursor, 3U);
}

TEST(EntityEditSystemTest, Update_IgnoresAnEditOfAnUnknownField)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kPlace, .text = "nowhere"};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_TRUE(store.ui.idField.text.empty());
}

TEST(EntityEditSystemTest, Update_RestoresTheBuffersOnCancel)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.selected = 0;
    store.ui.focus = widgets::kFieldId;
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId,
        .text = "typing",
        .cancelled = true};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.ui.idField.text, "keeper");
    EXPECT_EQ(store.ui.focus, antwika::ui::kNoWidget);
}

TEST(EntityEditSystemTest, Update_WritesTheBuffersBackOnSubmit)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(
        Transition{.id = "door", .targetMap = "old"});
    store.ui.selected = 0;
    store.ui.targetMapField.text = "next";
    store.ui.targetEntryField.text = "back";
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId,
        .text = "gate",
        .cursor = 4,
        .submitted = true};
    EntityEditSystem system{store};

    system.update(world, kTick);

    const auto &edited =
        std::get<Transition>(store.state.map.entities()[0]);
    EXPECT_EQ(edited.id, "gate");
    EXPECT_EQ(edited.targetMap, "next");
    EXPECT_EQ(edited.targetEntry, "back");
}

TEST(EntityEditSystemTest, Update_WritesAPickupsTagsBackOnSubmit)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Pickup{.id = "chest"});
    store.ui.selected = 0;
    store.ui.tagsField.text = "rowing, wet";
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId, .text = "chest", .submitted = true};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        std::get<Pickup>(store.state.map.entities()[0]).grantedTags,
        (std::vector<std::string>{"rowing", "wet"}));
}

TEST(EntityEditSystemTest, Update_SubmitsNothingWithoutASelection)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId, .text = "other", .submitted = true};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        std::get<Npc>(store.state.map.entities()[0]).id, "keeper");
}

TEST(EntityEditSystemTest, Update_SubmitsNothingPastTheEntityList)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    store.ui.selected = 5;
    store.ui.acted.edit = antwika::ui::TextEdit{
        .field = widgets::kFieldId, .text = "other", .submitted = true};
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(
        std::get<Npc>(store.state.map.entities()[0]).id, "keeper");
}

TEST(EntityEditSystemTest, Update_DoesNothingWithoutAnActionOrEdit)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    store.state.map.addEntity(Npc{.id = "keeper"});
    EntityEditSystem system{store};

    system.update(world, kTick);

    EXPECT_EQ(store.state.map.entities().size(), 1U);
    EXPECT_TRUE(store.ui.idField.text.empty());
}

TEST(EntityEditSystemTest, Update_WritesTheIdBackForEveryEntityKind)
{
    NiceMock<MockLogger> logger;
    World world{logger};
    auto store = storeOf();
    EntityEditSystem system{store};

    store.state.map.addEntity(Transition{.id = "a"});
    store.state.map.addEntity(
        antwika::tilemap::BoatEmbark{.id = "b"});
    store.state.map.addEntity(
        antwika::tilemap::SpawnPoint{.id = "c"});
    store.state.map.addEntity(Pickup{.id = "d"});
    store.state.map.addEntity(Npc{.id = "e"});
    store.state.map.addEntity(
        antwika::tilemap::TriggerVolume{.id = "f"});

    for (std::size_t at = 0; at < 6; ++at)
    {
        store.ui.selected = at;
        store.ui.idField.text = "renamed-" + std::to_string(at);
        store.ui.acted.edit = antwika::ui::TextEdit{
            .field = widgets::kFieldId,
            .text = "renamed-" + std::to_string(at),
            .submitted = true};

        system.update(world, kTick);

        EXPECT_EQ(
            antwika::map_editor::entityCellOf(
                store.state.map.entities()[at]),
            cellAt(0, 0));
    }

    EXPECT_EQ(store.state.map.entities().size(), 6U);
}
