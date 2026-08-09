#include <gtest/gtest.h>

#include <cstddef>
#include <optional>

#include "antwika/game/ServiceWalk.hpp"
#include "antwika/game/Service.hpp"
#include "antwika/game/Walker.hpp"

using antwika::game::carriesGoods;
using antwika::game::kWalkerKindCount;
using antwika::game::Service;
using antwika::game::serviceConferredBy;
using antwika::game::WalkerKind;

TEST(ServiceWalkTest, ServiceConferredBy_NamesTheServiceOfEveryWalker)
{
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::WaterCarrier), Service::Water);
    EXPECT_EQ(serviceConferredBy(WalkerKind::Doctor), Service::Health);

    EXPECT_EQ(serviceConferredBy(WalkerKind::Fireman), std::nullopt);
    EXPECT_EQ(serviceConferredBy(WalkerKind::Engineer), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::CartPusher), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::MarketBuyer), std::nullopt);
    EXPECT_EQ(
        serviceConferredBy(WalkerKind::MarketSeller), std::nullopt);
}

TEST(ServiceWalkTest, ServiceConferredBy_NeverAgreesWithCarryingGoods)
{
    for (std::size_t index = 0; index < kWalkerKindCount; ++index)
    {
        const auto kind = static_cast<WalkerKind>(index);

        EXPECT_FALSE(
            carriesGoods(kind) && serviceConferredBy(kind).has_value());
    }
}

TEST(ServiceWalkTest, ServiceConferredBy_WrapsAKindThatIsNotOne)
{
    const auto beyond = static_cast<WalkerKind>(kWalkerKindCount);

    EXPECT_EQ(serviceConferredBy(beyond), Service::Water);
}
