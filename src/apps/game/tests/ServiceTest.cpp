#include <gtest/gtest.h>

#include <cstddef>
#include <set>
#include <string_view>

#include "antwika/game/Service.hpp"
#include "antwika/game/Resource.hpp"

using antwika::game::kResources;
using antwika::game::kServiceCount;
using antwika::game::kServices;
using antwika::game::Resource;
using antwika::game::resourceName;
using antwika::game::Service;
using antwika::game::serviceIndex;
using antwika::game::serviceName;
using antwika::game::sustains;

TEST(ServiceTest, ServiceName_IsUniquePerService)
{
    std::set<std::string_view> names;

    for (std::size_t index = 0; index < kServiceCount; ++index)
    {
        const auto service = static_cast<Service>(index);

        EXPECT_EQ(serviceIndex(service), index);
        names.insert(serviceName(service));
    }

    EXPECT_EQ(names.size(), kServiceCount);
}

TEST(ServiceTest, ServiceIndex_ListsInItsOwnOrder)
{
    for (std::size_t index = 0; index < kServiceCount; ++index)
    {
        EXPECT_EQ(serviceIndex(kServices[index]), index);
    }
}

TEST(ServiceTest, ServiceName_NeverNamesAGood)
{
    std::set<std::string_view> goods;

    for (const auto resource : kResources)
    {
        goods.insert(resourceName(resource));
    }

    for (const auto service : kServices)
    {
        EXPECT_EQ(goods.count(serviceName(service)), 0U)
            << serviceName(service);
    }
}

TEST(ServiceTest, Sustains_IsTrueOnlyForFood)
{
    EXPECT_TRUE(sustains(Resource::Food));
    EXPECT_FALSE(sustains(Resource::Clay));
    EXPECT_FALSE(sustains(Resource::Pottery));
}
