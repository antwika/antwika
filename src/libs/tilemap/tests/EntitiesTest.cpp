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

namespace
{
    /**
     * @brief Checks that one member takes part in the comparison.
     */
    template <typename T, typename Mutate>
    void expectMemberCompared(const T &base, Mutate mutate)
    {
        T changed = base;
        mutate(changed);

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, base);
    }
}

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
        .level = 3,
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
    other.level = 4;
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

TEST(EntitiesTest, BoatEmbark_OperatorEquals_ComparesEveryField)
{
    const BoatEmbark base{
        .id = "jetty-north", .at = {.column = 4, .row = 2}, .level = 3};

    EXPECT_EQ(base, BoatEmbark{base});
    expectMemberCompared(base, [](auto &one) { one.id = "jetty-south"; });
    expectMemberCompared(base, [](auto &one) { one.at.column = 5; });
    expectMemberCompared(base, [](auto &one) { one.at.row = 6; });
    expectMemberCompared(base, [](auto &one) { one.level = 4; });
}

TEST(EntitiesTest, SpawnPoint_OperatorEquals_ComparesEveryField)
{
    const SpawnPoint base{
        .id = "den-west",
        .at = {.column = 4, .row = 2},
        .level = 3,
        .enemy = "crab"};

    EXPECT_EQ(base, SpawnPoint{base});
    expectMemberCompared(base, [](auto &one) { one.id = "den-east"; });
    expectMemberCompared(base, [](auto &one) { one.at.column = 5; });
    expectMemberCompared(base, [](auto &one) { one.at.row = 6; });
    expectMemberCompared(base, [](auto &one) { one.level = 4; });
    expectMemberCompared(base, [](auto &one) { one.enemy = "eel"; });
}

TEST(EntitiesTest, Pickup_OperatorEquals_ComparesEveryField)
{
    const Pickup base{
        .id = "chest-01",
        .at = {.column = 4, .row = 2},
        .level = 3,
        .item = "oar",
        .grantedTags = {"rowing"}};

    EXPECT_EQ(base, Pickup{base});
    expectMemberCompared(base, [](auto &one) { one.id = "chest-02"; });
    expectMemberCompared(base, [](auto &one) { one.at.column = 5; });
    expectMemberCompared(base, [](auto &one) { one.at.row = 6; });
    expectMemberCompared(base, [](auto &one) { one.level = 4; });
    expectMemberCompared(base, [](auto &one) { one.item = "sail"; });
    expectMemberCompared(
        base, [](auto &one) { one.grantedTags = {"sailing"}; });
}

TEST(EntitiesTest, Npc_OperatorEquals_ComparesEveryField)
{
    const Npc base{
        .id = "ferryman", .at = {.column = 4, .row = 2}, .level = 3};

    EXPECT_EQ(base, Npc{base});
    expectMemberCompared(base, [](auto &one) { one.id = "harbourmaster"; });
    expectMemberCompared(base, [](auto &one) { one.at.column = 5; });
    expectMemberCompared(base, [](auto &one) { one.at.row = 6; });
    expectMemberCompared(base, [](auto &one) { one.level = 4; });
}

TEST(EntitiesTest, TriggerVolume_OperatorEquals_ComparesEveryField)
{
    const TriggerVolume base{
        .id = "tide-watch",
        .at = {.column = 4, .row = 2},
        .level = 3,
        .columns = 2,
        .rows = 5,
        .event = "tide_rises",
        .grantedTags = {"wet"}};

    EXPECT_EQ(base, TriggerVolume{base});
    expectMemberCompared(base, [](auto &one) { one.id = "tide-ebbs"; });
    expectMemberCompared(base, [](auto &one) { one.at.column = 5; });
    expectMemberCompared(base, [](auto &one) { one.at.row = 6; });
    expectMemberCompared(base, [](auto &one) { one.level = 4; });
    expectMemberCompared(base, [](auto &one) { one.columns = 3; });
    expectMemberCompared(base, [](auto &one) { one.rows = 6; });
    expectMemberCompared(
        base, [](auto &one) { one.event = "tide_falls"; });
    expectMemberCompared(
        base, [](auto &one) { one.grantedTags = {"dry"}; });
}
