#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string_view>

#include "antwika/game/HousingLevel.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Store.hpp"

namespace
{
    using antwika::game::BuildingKind;
    using antwika::game::fetchesFromStores;
    using antwika::game::housesPeople;
    using antwika::game::HousingLevel;
    using antwika::game::housingLevelFromName;
    using antwika::game::housingLevelIndex;
    using antwika::game::housingLevelName;
    using antwika::game::kBuildingKindCount;
    using antwika::game::kHousingLevelCount;
    using antwika::game::kHousingLevels;
    using antwika::game::kHousingRequirements;
    using antwika::game::kResources;
    using antwika::game::kServiceCount;
    using antwika::game::kStockCapacity;
    using antwika::game::requirementOf;
    using antwika::game::Resource;
    using antwika::game::resourceIndex;
}

TEST(HousingLevelTest, HousingLevelName_IsDistinctForEveryLevel)
{
    std::set<std::string_view> named;

    for (const auto level : kHousingLevels)
    {
        named.insert(housingLevelName(level));
    }

    EXPECT_EQ(named.size(), kHousingLevelCount);
}

TEST(HousingLevelTest, HousingLevelFromName_RoundTripsEveryLevel)
{
    for (const auto level : kHousingLevels)
    {
        EXPECT_EQ(housingLevelFromName(housingLevelName(level)), level);
    }
}

TEST(HousingLevelTest, HousingLevelFromName_RefusesAnythingElse)
{
    EXPECT_FALSE(housingLevelFromName("").has_value());
    EXPECT_FALSE(housingLevelFromName("palace").has_value());
}

TEST(HousingLevelTest, RequirementOf_IndexesTheTableInEnumOrder)
{
    for (std::size_t index = 0; index < kHousingLevelCount; ++index)
    {
        const auto level = static_cast<HousingLevel>(index);

        EXPECT_EQ(housingLevelIndex(level), index);
        EXPECT_EQ(requirementOf(level), kHousingRequirements[index]);
    }
}

TEST(HousingLevelTest, KHousingRequirements_AsksNothingOfTheBottomLevel)
{
    const auto bottom = requirementOf(HousingLevel::Tent);

    EXPECT_EQ(bottom.desirability, 0);

    for (std::size_t slot = 0; slot < kServiceCount; ++slot)
    {
        EXPECT_FALSE(bottom.services[slot]);
    }

    for (const auto resource : kResources)
    {
        EXPECT_EQ(bottom.goods[resourceIndex(resource)], 0);
    }

    EXPECT_GT(bottom.populationCapacity, 0);
}

TEST(HousingLevelTest, KHousingRequirements_RisesWithEveryLevel)
{
    for (std::size_t index = 1; index < kHousingLevelCount; ++index)
    {
        const auto &below = kHousingRequirements[index - 1];
        const auto &here = kHousingRequirements[index];

        EXPECT_GE(here.desirability, below.desirability);
        EXPECT_GT(here.populationCapacity, below.populationCapacity);

        for (std::size_t slot = 0; slot < kServiceCount; ++slot)
        {
            EXPECT_TRUE(here.services[slot] || !below.services[slot]);
        }

        for (const auto resource : kResources)
        {
            EXPECT_GE(
                here.goods[resourceIndex(resource)],
                below.goods[resourceIndex(resource)]);
        }
    }
}

TEST(HousingLevelTest, KHousingRequirements_DemandsOnlyWhatReachesAHouse)
{
    std::size_t demands = 0;

    for (const auto level : kHousingLevels)
    {
        const auto wanted = requirementOf(level);

        for (const auto resource : kResources)
        {
            const auto asked = wanted.goods[resourceIndex(resource)];

            if (asked == 0)
            {
                continue;
            }

            ++demands;
            EXPECT_TRUE(fetchesFromStores(BuildingKind::Market, resource));
            EXPECT_LE(asked, kStockCapacity);
        }
    }

    EXPECT_GT(demands, 0U);
}

TEST(HousingLevelTest, KHousingRequirements_AsksTheTopLevelForPottery)
{
    const auto slot = resourceIndex(Resource::Pottery);

    EXPECT_GT(requirementOf(HousingLevel::Cottage).goods[slot], 0);
    EXPECT_EQ(requirementOf(HousingLevel::Hovel).goods[slot], 0);
}

TEST(HousingLevelTest, HousesPeople_NamesOnlyTheHouse)
{
    for (std::size_t index = 0; index < kBuildingKindCount; ++index)
    {
        const auto kind = static_cast<BuildingKind>(index);

        EXPECT_EQ(housesPeople(kind), kind == BuildingKind::House);
    }
}

TEST(HousingLevelTest, StockCapacityOf_GrowsOneShelfPerLevel)
{
    using antwika::game::kStockCapacity;
    using antwika::game::stockCapacityOf;

    EXPECT_EQ(stockCapacityOf(HousingLevel::Tent), kStockCapacity);
    EXPECT_EQ(
        stockCapacityOf(HousingLevel::Shack), 2 * kStockCapacity);
    EXPECT_EQ(
        stockCapacityOf(HousingLevel::Hovel), 3 * kStockCapacity);
    EXPECT_EQ(
        stockCapacityOf(HousingLevel::Cottage), 4 * kStockCapacity);
}
