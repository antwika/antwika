#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string_view>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Walker.hpp"

using antwika::game::BuildingKind;
using antwika::game::buildingKindFromName;
using antwika::game::buildingKindIndex;
using antwika::game::buildingKindName;
using antwika::game::BuildTool;
using antwika::game::buildingKindOf;
using antwika::game::carriesGoods;
using antwika::game::consumes;
using antwika::game::kBuildingKindCount;
using antwika::game::kBuildToolCount;
using antwika::game::kWalkerKindCount;
using antwika::game::sendsWalkers;
using antwika::game::WalkerKind;
using antwika::game::walkerSentBy;

TEST(BuildingKindTest, Consumes_OnlyAHouseConsumes)
{
    EXPECT_TRUE(consumes(BuildingKind::House));

    for (std::size_t index = 1; index < kBuildingKindCount; ++index)
    {
        EXPECT_FALSE(consumes(static_cast<BuildingKind>(index))) << index;
    }
}

TEST(BuildingKindTest, SendsWalkers_AStorehouseNeitherEatsNorWalks)
{
    EXPECT_FALSE(consumes(BuildingKind::Storage));
    EXPECT_FALSE(sendsWalkers(BuildingKind::Storage));
}

TEST(BuildingKindTest, SendsWalkers_ExemptsHouseAndStorehouse)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);
        const auto walks = kind != BuildingKind::House
            && kind != BuildingKind::Storage;

        EXPECT_EQ(sendsWalkers(kind), walks) << index;
    }
}

TEST(BuildingKindTest, BuildingKindName_IsUniquePerKind)
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

TEST(BuildingKindTest, BuildingKindFromName_AnUnknownNameNamesNoKind)
{
    EXPECT_FALSE(buildingKindFromName("tower").has_value());
    EXPECT_FALSE(buildingKindFromName("").has_value());
    EXPECT_FALSE(buildingKindFromName("food_source").has_value());
    EXPECT_FALSE(buildingKindFromName("architect_post").has_value());
}

TEST(BuildingKindTest, BuildingKindOf_EveryKindIsPlacedByExactlyOneTool)
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

TEST(WalkerKindTest, CarriesGoods_NamesTheHaulersAndTheMarketWalkers)
{
    EXPECT_TRUE(carriesGoods(WalkerKind::CartPusher));
    EXPECT_TRUE(carriesGoods(WalkerKind::MarketBuyer));
    EXPECT_TRUE(carriesGoods(WalkerKind::MarketSeller));

    for (std::size_t index = 0; index < kWalkerKindCount; ++index)
    {
        const auto kind = static_cast<WalkerKind>(index);

        if (kind == WalkerKind::CartPusher
            || kind == WalkerKind::MarketBuyer
            || kind == WalkerKind::MarketSeller)
        {
            continue;
        }

        EXPECT_FALSE(carriesGoods(kind)) << index;
    }
}

TEST(WalkerKindTest, WalkerSentBy_MatchesEachSendersKind)
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

TEST(WalkerKindTest, WalkerSentBy_AgreesWithSendsWalkers)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(sendsWalkers(kind), walkerSentBy(kind).has_value())
            << index;
    }
}

TEST(WalkerKindTest, WalkerSentBy_NoCadenceSendsAMarketBuyer)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        EXPECT_NE(
            walkerSentBy(static_cast<BuildingKind>(index)),
            WalkerKind::MarketBuyer)
            << index;
    }
}
