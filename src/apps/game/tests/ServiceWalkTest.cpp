#include "antwika/game/ServiceWalk.hpp"

#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::game::carriedResource;
using antwika::game::kWalkerKindCount;
using antwika::game::Service;
using antwika::game::serviceConferredBy;
using antwika::game::WalkerKind;

// The table is the feature, so the test is exhaustive over it.
TEST(ServiceWalkTest, ServiceConferredBy_NamesTheServiceOfEveryWalker)
{
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::WaterCarrier), Service::Water);
    EXPECT_EQ(serviceConferredBy(WalkerKind::Doctor), Service::Health);

    // A fireman and an engineer relieve a risk directly instead.
    // See BuildingSystem's relief pass.
    EXPECT_EQ(serviceConferredBy(WalkerKind::Fireman), std::nullopt);
    EXPECT_EQ(serviceConferredBy(WalkerKind::Engineer), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::CartPusher), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::MarketBuyer), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::MarketSeller), std::nullopt);
}

// A good changes hands and a service is refreshed.
// Nobody does both, which two tables are where it could stop being so.
TEST(ServiceWalkTest, ServiceConferredBy_NeverAgreesWithCarriedResource)
{
    for (std::size_t index = 0; index < kWalkerKindCount; ++index)
    {
        const auto kind = static_cast<WalkerKind>(index);

        EXPECT_FALSE(
            carriedResource(kind).has_value()
            && serviceConferredBy(kind).has_value());
    }
}

// An out-of-range kind is a table read, not a crash.
// The same total-lookup rule every other table here follows.
TEST(ServiceWalkTest, ServiceConferredBy_WrapsAKindThatIsNotOne)
{
    const auto beyond = static_cast<WalkerKind>(kWalkerKindCount);

    EXPECT_EQ(serviceConferredBy(beyond), Service::Water);
}
