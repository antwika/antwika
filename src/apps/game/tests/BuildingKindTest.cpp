#include "antwika/game/BuildingKind.hpp"

#include <gtest/gtest.h>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Walker.hpp"

using antwika::game::BuildingKind;
using antwika::game::buildingKindFromName;
using antwika::game::buildingKindIndex;
using antwika::game::buildingKindName;
using antwika::game::BuildTool;
using antwika::game::buildingKindOf;
using antwika::game::carriedResource;
using antwika::game::consumes;
using antwika::game::kBuildingKindCount;
using antwika::game::Resource;
using antwika::game::sendsWalkers;
using antwika::game::WalkerKind;
using antwika::game::walkerSentBy;

TEST(BuildingKindTest, OnlyAHouseConsumes)
{
    EXPECT_TRUE(consumes(BuildingKind::House));
    EXPECT_FALSE(consumes(BuildingKind::FoodSource));
    EXPECT_FALSE(consumes(BuildingKind::ArchitectPost));
}

TEST(BuildingKindTest, EveryKindButAHouseSendsWalkers)
{
    EXPECT_FALSE(sendsWalkers(BuildingKind::House));
    EXPECT_TRUE(sendsWalkers(BuildingKind::WaterSource));
    EXPECT_TRUE(sendsWalkers(BuildingKind::FireStation));
}

TEST(BuildingKindTest, EveryKindHasItsOwnNameAndIndex)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(buildingKindIndex(kind), index);
        EXPECT_EQ(buildingKindFromName(buildingKindName(kind)), kind);
    }
}

// A save file writes the name.
// So an unknown one has to be refusable rather than become a kind.
TEST(BuildingKindTest, AnUnknownNameNamesNoKind)
{
    EXPECT_FALSE(buildingKindFromName("tower").has_value());
    EXPECT_FALSE(buildingKindFromName("").has_value());
}

TEST(BuildingKindTest, EveryToolButTheRoadPlacesItsMatchingKind)
{
    EXPECT_FALSE(buildingKindOf(BuildTool::Road).has_value());
    EXPECT_EQ(buildingKindOf(BuildTool::House), BuildingKind::House);
    EXPECT_EQ(
        buildingKindOf(BuildTool::FireStation), BuildingKind::FireStation);
}

TEST(WalkerKindTest, OnlyTheCarryingKindsCarryAnything)
{
    EXPECT_EQ(carriedResource(WalkerKind::Food), Resource::Food);
    EXPECT_EQ(carriedResource(WalkerKind::Water), Resource::Water);
    EXPECT_FALSE(carriedResource(WalkerKind::Fireman).has_value());
    EXPECT_FALSE(carriedResource(WalkerKind::Architect).has_value());
}

TEST(WalkerKindTest, EverySourceSendsItsOwnKindAndAHouseSendsNobody)
{
    EXPECT_FALSE(walkerSentBy(BuildingKind::House).has_value());
    EXPECT_EQ(walkerSentBy(BuildingKind::FoodSource), WalkerKind::Food);
    EXPECT_EQ(walkerSentBy(BuildingKind::WaterSource), WalkerKind::Water);
    EXPECT_EQ(walkerSentBy(BuildingKind::FireStation), WalkerKind::Fireman);
    EXPECT_EQ(
        walkerSentBy(BuildingKind::ArchitectPost), WalkerKind::Architect);
}
