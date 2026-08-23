#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/app/FramePacingStats.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/time/fakes/FakeClock.hpp>

using antwika::app::FramePacingReport;
using antwika::app::FramePacingStats;
using antwika::app::getFormatPacingReport;
using antwika::log::Level;
using antwika::log::mocks::MockLogger;
using antwika::time::fakes::FakeClock;
using ::testing::_;
using ::testing::NiceMock;
using std::chrono::microseconds;
using std::chrono::milliseconds;

namespace
{
    const auto kEpoch =
        std::chrono::time_point<std::chrono::system_clock>{};

    constexpr milliseconds kWindow{1000};

    struct PacingHarness final
    {
        FakeClock clock{kEpoch};
        NiceMock<MockLogger> logger;
        FramePacingStats traceStats{clock, logger, kWindow};
        std::vector<std::string> lines;

        PacingHarness()
        {
            ON_CALL(logger, log(_, _))
                .WillByDefault(
                    [this](Level, std::string_view message)
                    { lines.emplace_back(message); });
        }

        void at(microseconds when)
        {
            clock.set(kEpoch + when);
        }

        void closeWindow()
        {
            at(microseconds{kWindow});
            traceStats.onFrameDrawn(0);
        }
    };
}

TEST(FramePacingStatsTest, OnFrameDrawn_SaysNothingBeforeTheWindowElapses)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);
    harness.at(microseconds{500});
    harness.traceStats.onFrameDrawn(0);

    EXPECT_TRUE(harness.lines.empty());
}

TEST(FramePacingStatsTest, OnFrameDrawn_CountsTheFramesDrawnAndTheFramesDropped)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDropped(0);
    harness.traceStats.onFrameDropped(0);
    harness.traceStats.onFrameDrawn(0);

    harness.closeWindow();

    ASSERT_EQ(harness.lines.size(), 3U);
    EXPECT_EQ(harness.lines[0], "pacing drawn 3 of 5 over ticks 0");
}

TEST(FramePacingStatsTest, OnFrameDropped_FindsTheLongestRunOfDropsInARow)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDropped(0);
    harness.traceStats.onFrameDropped(0);
    harness.traceStats.onFrameDropped(0);
    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDropped(0);

    harness.closeWindow();

    ASSERT_EQ(harness.lines.size(), 3U);
    EXPECT_EQ(
        harness.lines[1],
        "pacing longest drop run 3, leanest tick 0, fattest tick 0");
}

TEST(FramePacingStatsTest, OnFrameDrawn_SortsTheGapsBetweenFramesIntoBuckets)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);

    harness.at(microseconds{400});
    harness.traceStats.onFrameDrawn(0);

    harness.at(microseconds{1900});
    harness.traceStats.onFrameDrawn(0);

    harness.at(microseconds{5900});
    harness.traceStats.onFrameDrawn(0);

    harness.at(microseconds{11900});
    harness.traceStats.onFrameDrawn(0);

    harness.at(microseconds{21900});
    harness.traceStats.onFrameDrawn(0);

    harness.closeWindow();

    ASSERT_EQ(harness.lines.size(), 3U);
    EXPECT_EQ(
        harness.lines[2],
        "pacing gaps <1ms:1 1ms:1 2ms:0 3ms:0 4ms:1 5-7ms:1 8-15ms:1 "
        ">=16ms:1 ");
}

TEST(
    FramePacingStatsTest,
    OnFrameDrawn_ReportsTheMinAndMaxFramesPerTickItClosed)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDrawn(0);

    harness.traceStats.onFrameDrawn(1);

    harness.traceStats.onFrameDrawn(2);
    harness.traceStats.onFrameDrawn(2);

    harness.at(microseconds{kWindow});
    harness.traceStats.onFrameDrawn(3);

    ASSERT_EQ(harness.lines.size(), 3U);
    EXPECT_EQ(
        harness.lines[1],
        "pacing longest drop run 0, leanest tick 1, fattest tick 3");
}

TEST(FramePacingStatsTest, OnFrameDrawn_StartsAFreshReportOnceOneIsSaid)
{
    PacingHarness harness;

    harness.traceStats.onFrameDrawn(0);
    harness.traceStats.onFrameDropped(0);

    harness.closeWindow();

    ASSERT_EQ(harness.lines.size(), 3U);

    harness.at(microseconds{kWindow} * 2);
    harness.traceStats.onFrameDrawn(0);

    ASSERT_EQ(harness.lines.size(), 6U);
    EXPECT_EQ(harness.lines[3], "pacing drawn 1 of 1 over ticks 0");
}

TEST(FramePacingStatsTest, FormatPacingReport_SayWhatWasDrawnOfWhatWasScheduled)
{
    FramePacingReport report{};
    report.drawnFrames = 23;
    report.droppedFrames = 17;
    report.ticks = 1;

    const auto lines = getFormatPacingReport(report);

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], "pacing drawn 23 of 40 over ticks 1");
}

TEST(FramePacingStatsTest, FormatPacingReport_NameEveryBucketAndItsCount)
{
    FramePacingReport report{};
    report.intervals[0] = 5;
    report.intervals[7] = 2;

    const auto lines = getFormatPacingReport(report);

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(
        lines[2],
        "pacing gaps <1ms:5 1ms:0 2ms:0 3ms:0 4ms:0 5-7ms:0 8-15ms:0 "
        ">=16ms:2 ");
}

TEST(FramePacingStatsTest, FormatPacingReport_ReportTheRunsAndTheTickExtremes)
{
    FramePacingReport report{};
    report.longestDropRun = 9;
    report.minFramesPerTick = 12;
    report.maxFramesPerTick = 31;

    const auto lines = getFormatPacingReport(report);

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(
        lines[1],
        "pacing longest drop run 9, leanest tick 12, fattest tick 31");
}
