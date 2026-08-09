#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <optional>

#include <antwika/time/fakes/FakeClock.hpp>

#include "antwika/game/FrameMeter.hpp"

using antwika::game::FrameMeter;
using antwika::time::fakes::FakeClock;

namespace
{
    using Clock = std::chrono::system_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    constexpr TimePoint kStart{};

    [[nodiscard]] TimePoint at(std::int64_t millis)
    {
        return kStart + std::chrono::milliseconds{millis};
    }

    class FrameMeterTest : public ::testing::Test
    {
    protected:
        void drawFor(std::uint32_t frames, std::int64_t millis)
        {
            for (std::uint32_t frame = 1; frame <= frames; ++frame)
            {
                clock.set(
                    at(elapsed
                       + static_cast<std::int64_t>(frame) * millis
                             / static_cast<std::int64_t>(frames)));
                meter.record();
            }

            elapsed += millis;
        }

        FakeClock clock{kStart};
        FrameMeter meter{clock};
        std::int64_t elapsed = 0;
    };
}

TEST_F(FrameMeterTest, PerSecond_ReportsNothingBeforeAWindowIsUp)
{
    meter.record();
    drawFor(24, 960);

    EXPECT_EQ(std::nullopt, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_ReportsAStalledMachineAsZero)
{
    meter.record();
    drawFor(1, 2000);

    EXPECT_EQ(0U, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_ReportsTheFramesTheLastSecondHeld)
{
    meter.record();
    drawFor(25, 1000);

    EXPECT_EQ(25U, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_ReportsAFasterMachineAsFaster)
{
    meter.record();
    drawFor(100, 1000);

    EXPECT_EQ(100U, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_MeasuresEachWindowOnItsOwn)
{
    meter.record();
    drawFor(60, 1000);
    ASSERT_EQ(60U, meter.perSecond());

    drawFor(30, 1000);

    EXPECT_EQ(30U, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_DividesByTheSpanItActuallyMeasured)
{
    meter.record();
    clock.set(at(750));
    meter.record();
    clock.set(at(1500));
    meter.record();

    EXPECT_EQ(1U, meter.perSecond());
}

TEST_F(FrameMeterTest, PerSecond_KeepsTheLastAnswerMidWindow)
{
    meter.record();
    drawFor(40, 1000);

    drawFor(1, 10);

    EXPECT_EQ(40U, meter.perSecond());
}
