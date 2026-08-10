#include <gtest/gtest.h>

#include <variant>

#include <antwika/tilemap/Entities.hpp>

using antwika::tilemap::BoatEmbark;
using antwika::tilemap::Entity;
using antwika::tilemap::Npc;
using antwika::tilemap::Pickup;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::Transition;
using antwika::tilemap::TriggerVolume;

TEST(EntitiesTest, Entity_AdmitsEachOfTheSixKinds)
{
    EXPECT_TRUE(std::holds_alternative<Transition>(Entity{Transition{}}));
    EXPECT_TRUE(std::holds_alternative<BoatEmbark>(Entity{BoatEmbark{}}));
    EXPECT_TRUE(std::holds_alternative<SpawnPoint>(Entity{SpawnPoint{}}));
    EXPECT_TRUE(std::holds_alternative<Pickup>(Entity{Pickup{}}));
    EXPECT_TRUE(std::holds_alternative<Npc>(Entity{Npc{}}));
    EXPECT_TRUE(
        std::holds_alternative<TriggerVolume>(Entity{TriggerVolume{}}));
}

TEST(EntitiesTest, TriggerVolume_DefaultsToASingleCellRegion)
{
    const TriggerVolume trigger{};

    EXPECT_EQ(trigger.columns, 1U);
    EXPECT_EQ(trigger.rows, 1U);
}

TEST(EntitiesTest, Transition_OperatorEquals_ComparesEveryField)
{
    const Transition base{
        .id = "door-east",
        .at = {.column = 4, .row = 2},
        .targetMap = "wakewater-02",
        .targetEntry = "door-west",
        .requiredTags = {"boss_key"}};
    const auto twin = base;

    EXPECT_EQ(base, twin);

    auto other = base;
    other.id = "door-north";
    EXPECT_NE(base, other);

    other = base;
    other.at.column = 5;
    EXPECT_NE(base, other);

    other = base;
    other.targetMap = "wakewater-03";
    EXPECT_NE(base, other);

    other = base;
    other.targetEntry = "door-south";
    EXPECT_NE(base, other);

    other = base;
    other.requiredTags = {"swim"};
    EXPECT_NE(base, other);
}
