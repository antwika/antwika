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
            .day = 3,
            .hunger = 3,
            .fun = 5,
            .happiness = 7,
            .energy = 12,
            .energyCeiling = 30,
            .meals = 4,
            .plays = 6,
            .disturbances = 1,
            .pesters = 2,
            .collapses = 1,
            .generation = 2,
            .bestTicks = 400,
            .perished = false};

        const auto line = summaryLine(summary);

        EXPECT_THAT(line, HasSubstr("still with us"));
        EXPECT_THAT(line, HasSubstr("120 ticks"));
        EXPECT_THAT(line, HasSubstr("3 days"));
        EXPECT_THAT(line, HasSubstr("4 meals"));
        EXPECT_THAT(line, HasSubstr("6 games"));
        EXPECT_THAT(line, HasSubstr("1 rude awakenings"));
        EXPECT_THAT(line, HasSubstr("2 unwanted attentions"));
        EXPECT_THAT(line, HasSubstr("1 collapses"));
        EXPECT_THAT(line, HasSubstr("energy 12/30"));
        EXPECT_THAT(line, HasSubstr("best so far 400 ticks"));
        EXPECT_THAT(line, HasSubstr("companion number 2"));
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
