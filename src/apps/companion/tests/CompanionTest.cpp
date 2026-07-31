#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/companion/Companion.hpp"

using antwika::companion::announceHowToStop;
using antwika::companion::CompanionSummary;
using antwika::companion::summaryLine;
using antwika::log::mocks::MockLogger;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;

namespace
{
    TEST(AnnounceHowToStopTest, SaysNothingWhenThereIsAWindowToClose)
    {
        NiceMock<MockLogger> logger;

        EXPECT_CALL(logger, log(_, _)).Times(0);

        announceHowToStop(logger, false);
    }

    TEST(AnnounceHowToStopTest, SaysHowToStopAHeadlessSession)
    {
        NiceMock<MockLogger> logger;

        EXPECT_CALL(logger, log(_, HasSubstr("press Ctrl+C to stop")));

        announceHowToStop(logger, true);
    }

    TEST(SummaryLineTest, ASurvivingCompanionIsSaidToBeStillWithUs)
    {
        const CompanionSummary summary{
            .ticks = 120,
            .hunger = 3,
            .happiness = 7,
            .meals = 4,
            .disturbances = 1,
            .perished = false};

        const auto line = summaryLine(summary);

        EXPECT_THAT(line, HasSubstr("still with us"));
        EXPECT_THAT(line, HasSubstr("120 ticks"));
        EXPECT_THAT(line, HasSubstr("4 meals"));
        EXPECT_THAT(line, HasSubstr("1 rude awakenings"));
        EXPECT_THAT(line, HasSubstr("happiness 7"));
        EXPECT_THAT(line, HasSubstr("hunger 3"));
    }

    TEST(SummaryLineTest, APerishedCompanionIsSaidToHavePerished)
    {
        const CompanionSummary summary{.ticks = 900, .perished = true};

        const auto line = summaryLine(summary);

        EXPECT_THAT(line, HasSubstr("perished"));
        EXPECT_THAT(line, HasSubstr("900 ticks"));
    }

    // Two constants that have to agree with each other.
    // One is how fast a companion lives, the other how long a tick is.
    TEST(TickIntervalTest, OneSecondOfTicksIsOneSecondOfWallClock)
    {
        EXPECT_EQ(
            antwika::companion::kTickInterval.count()
                * static_cast<long long>(
                    antwika::companion::kTicksPerSecond),
            1000);
    }
} // namespace
