#include <gtest/gtest.h>

#include "antwika/network/DeliverySchedule.hpp"

using antwika::network::DeliverySchedule;

TEST(DeliveryScheduleTest, Ctor_DefaultsToCarryingEverythingAtOnce)
{
    const DeliverySchedule prompt;

    EXPECT_EQ(prompt.delayPumps, 0U);
    EXPECT_TRUE(prompt.dropped.empty());
}

TEST(DeliveryScheduleTest, OperatorEquals_ComparesEveryFieldIndependently)
{
    const DeliverySchedule schedule{.delayPumps = 2, .dropped = {1, 3}};

    EXPECT_EQ(
        schedule,
        (DeliverySchedule{.delayPumps = 2, .dropped = {1, 3}}));

    EXPECT_NE(
        schedule,
        (DeliverySchedule{.delayPumps = 3, .dropped = {1, 3}}));

    EXPECT_NE(
        schedule,
        (DeliverySchedule{.delayPumps = 2, .dropped = {1, 4}}));

    EXPECT_NE(
        schedule, (DeliverySchedule{.delayPumps = 2, .dropped = {}}));
}
