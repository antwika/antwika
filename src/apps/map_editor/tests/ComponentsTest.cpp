#include <gtest/gtest.h>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Entities.hpp>

#include "antwika/map_editor/Components.hpp"

using antwika::geometry::GridCell;
using antwika::map_editor::entityCellOf;
using antwika::map_editor::entityLevelOf;
using antwika::map_editor::kMarkerKindCount;
using antwika::map_editor::kMarkerKindNames;
using antwika::map_editor::markerKindOf;
using antwika::map_editor::MarkerKind;
using antwika::tilemap::BoatEmbark;
using antwika::tilemap::Entity;
using antwika::tilemap::Npc;
using antwika::tilemap::Pickup;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::Transition;
using antwika::tilemap::TriggerVolume;

namespace
{
    constexpr GridCell kSomewhere{.column = 4, .row = 7};
}

TEST(ComponentsTest, MarkerKindOf_NamesTheKindOfEveryEntity)
{
    EXPECT_EQ(markerKindOf(Entity{Transition{}}), MarkerKind::Transition);
    EXPECT_EQ(markerKindOf(Entity{BoatEmbark{}}), MarkerKind::Boat);
    EXPECT_EQ(markerKindOf(Entity{SpawnPoint{}}), MarkerKind::Spawn);
    EXPECT_EQ(markerKindOf(Entity{Pickup{}}), MarkerKind::Pickup);
    EXPECT_EQ(markerKindOf(Entity{Npc{}}), MarkerKind::Npc);
    EXPECT_EQ(markerKindOf(Entity{TriggerVolume{}}), MarkerKind::Trigger);
}

TEST(ComponentsTest, MarkerKindNames_HoldsOneNamePerKind)
{
    EXPECT_EQ(kMarkerKindNames.size(), kMarkerKindCount);
    EXPECT_EQ(kMarkerKindNames[0], "transition");
    EXPECT_EQ(kMarkerKindNames[5], "trigger");
}

TEST(ComponentsTest, EntityCellOf_ReadsTheCellOfEveryEntityKind)
{
    EXPECT_EQ(
        entityCellOf(Entity{Transition{.at = kSomewhere}}), kSomewhere);
    EXPECT_EQ(
        entityCellOf(Entity{BoatEmbark{.at = kSomewhere}}), kSomewhere);
    EXPECT_EQ(
        entityCellOf(Entity{SpawnPoint{.at = kSomewhere}}), kSomewhere);
    EXPECT_EQ(
        entityCellOf(Entity{Pickup{.at = kSomewhere}}), kSomewhere);
    EXPECT_EQ(entityCellOf(Entity{Npc{.at = kSomewhere}}), kSomewhere);
    EXPECT_EQ(
        entityCellOf(Entity{TriggerVolume{.at = kSomewhere}}),
        kSomewhere);
}

TEST(ComponentsTest, EntityLevelOf_ReadsTheLevelOfEveryEntityKind)
{
    EXPECT_EQ(entityLevelOf(Entity{Transition{.level = -3}}), -3);
    EXPECT_EQ(entityLevelOf(Entity{BoatEmbark{.level = 1}}), 1);
    EXPECT_EQ(entityLevelOf(Entity{SpawnPoint{.level = 2}}), 2);
    EXPECT_EQ(entityLevelOf(Entity{Pickup{.level = 3}}), 3);
    EXPECT_EQ(entityLevelOf(Entity{Npc{.level = 4}}), 4);
    EXPECT_EQ(entityLevelOf(Entity{TriggerVolume{.level = 5}}), 5);
}
