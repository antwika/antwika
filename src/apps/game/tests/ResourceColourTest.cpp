#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include <antwika/gfx/Color.hpp>

#include "antwika/game/Resource.hpp"
#include "antwika/game/ResourceColour.hpp"
#include "antwika/game/Service.hpp"

using antwika::game::kResourceCount;
using antwika::game::kResources;
using antwika::game::kServiceCount;
using antwika::game::kServices;
using antwika::game::resourceColour;
using antwika::game::serviceColour;

namespace
{
    [[nodiscard]] std::uint32_t packed(antwika::gfx::Color colour)
    {
        return (static_cast<std::uint32_t>(colour.red) << 16)
               | (static_cast<std::uint32_t>(colour.green) << 8)
               | static_cast<std::uint32_t>(colour.blue);
    }
} // namespace

TEST(ResourceColourTest, GivesEachResourceOneOfItsOwn)
{
    std::set<std::uint32_t> seen;

    for (const auto resource : kResources)
    {
        const auto colour = resourceColour(resource);
        seen.insert(packed(colour));

        // Opaque, so a line reads as ink rather than as a shadow.
        EXPECT_EQ(colour.alpha, 255);
    }

    EXPECT_EQ(seen.size(), kResourceCount);
}

TEST(ResourceColourTest, GivesEachServiceOneOfItsOwn)
{
    std::set<std::uint32_t> seen;

    for (const auto service : kServices)
    {
        const auto colour = serviceColour(service);
        seen.insert(packed(colour));

        EXPECT_EQ(colour.alpha, 255);
    }

    EXPECT_EQ(seen.size(), kServiceCount);
}

TEST(ResourceColourTest, TellsAResourceApartFromEveryService)
{
    std::set<std::uint32_t> seen;

    for (const auto resource : kResources)
    {
        seen.insert(packed(resourceColour(resource)));
    }

    for (const auto service : kServices)
    {
        seen.insert(packed(serviceColour(service)));
    }

    EXPECT_EQ(seen.size(), kResourceCount + kServiceCount);
}
