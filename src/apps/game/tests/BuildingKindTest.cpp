#include "antwika/game/BuildingKind.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string_view>

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
using antwika::game::kBuildToolCount;
using antwika::game::kWalkerKindCount;
using antwika::game::Resource;
using antwika::game::sendsWalkers;
using antwika::game::WalkerKind;
using antwika::game::walkerSentBy;

TEST(BuildingKindTest, OnlyAHouseConsumes)
{
    EXPECT_TRUE(consumes(BuildingKind::House));

    for (std::size_t index = 1; index < kBuildingKindCount; ++index)
    {
        EXPECT_FALSE(consumes(static_cast<BuildingKind>(index))) << index;
    }
}

// The two predicates used to be each other's negation.
// A storehouse is what stopped that being true.
TEST(BuildingKindTest, AStorehouseNeitherEatsNorWalks)
{
    EXPECT_FALSE(consumes(BuildingKind::Storage));
    EXPECT_FALSE(sendsWalkers(BuildingKind::Storage));
}

TEST(BuildingKindTest, EveryKindSendsWalkersButTheHouseAndTheStorehouse)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);
        const auto walks = kind != BuildingKind::House
            && kind != BuildingKind::Storage;

        EXPECT_EQ(sendsWalkers(kind), walks) << index;
    }
}

TEST(BuildingKindTest, EveryKindHasItsOwnNameAndIndex)
{
    std::set<std::string_view> names;

    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(buildingKindIndex(kind), index);
        EXPECT_EQ(buildingKindFromName(buildingKindName(kind)), kind);
        names.insert(buildingKindName(kind));
    }

    EXPECT_EQ(names.size(), kBuildingKindCount);
}

// A save file writes the name.
// So an unknown one has to be refusable rather than become a kind.
// The two version 2 names are here because a file may still hold them.
// Refusing them is what makes the migration the only way in.
TEST(BuildingKindTest, AnUnknownNameNamesNoKind)
{
    EXPECT_FALSE(buildingKindFromName("tower").has_value());
    EXPECT_FALSE(buildingKindFromName("").has_value());
    EXPECT_FALSE(buildingKindFromName("food_source").has_value());
    EXPECT_FALSE(buildingKindFromName("architect_post").has_value());
}

TEST(BuildingKindTest, EveryKindIsPlacedByExactlyOneTool)
{
    for (std::size_t kind = 0; kind < kBuildingKindCount; ++kind)
    {
        std::size_t placing = 0;

        for (std::size_t tool = 0; tool < kBuildToolCount; ++tool)
        {
            if (buildingKindOf(static_cast<BuildTool>(tool))
                == static_cast<BuildingKind>(kind))
            {
                ++placing;
            }
        }

        EXPECT_EQ(placing, 1U) << kind;
    }
}

TEST(WalkerKindTest, OnlyTheSellerCarriesWhatItsKindDecides)
{
    EXPECT_EQ(carriedResource(WalkerKind::MarketSeller), Resource::Food);

    for (std::size_t index = 0; index < kWalkerKindCount; ++index)
    {
        const auto kind = static_cast<WalkerKind>(index);

        if (kind == WalkerKind::MarketSeller)
        {
            continue;
        }

        EXPECT_FALSE(carriedResource(kind).has_value()) << index;
    }
}

TEST(WalkerKindTest, EverySenderSendsItsOwnKindAndTheRestSendNobody)
{
    EXPECT_FALSE(walkerSentBy(BuildingKind::House).has_value());
    EXPECT_FALSE(walkerSentBy(BuildingKind::Storage).has_value());
    EXPECT_EQ(walkerSentBy(BuildingKind::Farm), WalkerKind::CartPusher);
    EXPECT_EQ(walkerSentBy(BuildingKind::ClayPit), WalkerKind::CartPusher);
    EXPECT_EQ(walkerSentBy(BuildingKind::Workshop), WalkerKind::CartPusher);
    EXPECT_EQ(walkerSentBy(BuildingKind::Market), WalkerKind::MarketSeller);
    EXPECT_EQ(walkerSentBy(BuildingKind::Well), WalkerKind::WaterCarrier);
    EXPECT_EQ(walkerSentBy(BuildingKind::Doctor), WalkerKind::Doctor);
    EXPECT_EQ(walkerSentBy(BuildingKind::FireStation), WalkerKind::Fireman);
    EXPECT_EQ(
        walkerSentBy(BuildingKind::EngineerPost), WalkerKind::Engineer);
}

// The table and the predicate are two statements of one fact.
// So the one place they can disagree is asserted over every kind.
TEST(WalkerKindTest, SendingAWalkerAndNamingOneAgreeOnEveryKind)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(sendsWalkers(kind), walkerSentBy(kind).has_value())
            << index;
    }
}

// A market buyer is sent by an errand rather than by a cadence.
// So no building names it, and that is deliberate rather than a gap.
TEST(WalkerKindTest, NoCadenceSendsAMarketBuyer)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        EXPECT_NE(
            walkerSentBy(static_cast<BuildingKind>(index)),
            WalkerKind::MarketBuyer)
            << index;
    }
}
