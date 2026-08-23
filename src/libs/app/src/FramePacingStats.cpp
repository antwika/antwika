#include "antwika/app/FramePacingStats.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/log/Level.hpp>

namespace antwika::app
{

    namespace
    {
        constexpr std::array<const char *, kPacingBuckets> kBucketNames{
            "<1ms",
            "1ms",
            "2ms",
            "3ms",
            "4ms",
            "5-7ms",
            "8-15ms",
            ">=16ms"};

        [[nodiscard]] std::size_t bucketFor(
            std::chrono::microseconds gap) noexcept
        {
            const auto millis = gap.count() / 1000;

            if (millis < 1)
            {
                return 0;
            }

            if (millis <= 4)
            {
                return static_cast<std::size_t>(millis);
            }

            if (millis <= 7)
            {
                return 5;
            }

            if (millis <= 15)
            {
                return 6;
            }

            return 7;
        }

        [[nodiscard]] std::string getCountedLabel(
            const char *name, std::uint32_t amount)
        {
            return std::string(name) + " " + std::to_string(amount);
        }
    }

    std::vector<std::string> getFormatPacingReport(const FramePacingReport &report)
    {
        const auto scheduledFrames = report.drawnFrames + report.droppedFrames;

        std::string spread = "pacing gaps ";

        for (std::size_t index = 0; index < kPacingBuckets; ++index)
        {
            spread += std::string(kBucketNames[index]) + ":"
                      + std::to_string(report.intervals[index]) + " ";
        }

        std::vector<std::string> lines;

        lines.push_back(
            "pacing " + getCountedLabel("drawn", report.drawnFrames) + " of "
            + std::to_string(scheduledFrames) + " over "
            + getCountedLabel("ticks", report.ticks));

        lines.push_back(
            "pacing " + getCountedLabel("longest drop run", report.longestDropRun)
            + ", " + getCountedLabel("leanest tick", report.minFramesPerTick)
            + ", " + getCountedLabel("fattest tick", report.maxFramesPerTick));

        lines.push_back(std::move(spread));

        return lines;
    } // GCOVR_EXCL_LINE

    FramePacingStats::FramePacingStats(
        const antwika::time::IClock &clock,
        antwika::log::ILogger &logger,
        std::chrono::milliseconds window) noexcept
        : clock(clock), logger(logger), window(window)
    {
    }

    void FramePacingStats::closeWindow()
    {
        for (const auto &line : getFormatPacingReport(currentReport))
        {
            logger.log(antwika::log::Level::Info, line);
        }

        currentReport = FramePacingReport{};
    }

    void FramePacingStats::closeTick(antwika::time::Tick tick)
    {
        if (openTick.has_value() && *openTick == tick)
        {
            return;
        }

        if (openTick.has_value())
        {
            ++currentReport.ticks;

            currentReport.maxFramesPerTick =
                std::max(currentReport.maxFramesPerTick, drawnThisTick);

            currentReport.minFramesPerTick = currentReport.ticks == 1
                                           ? drawnThisTick
                : std::min(currentReport.minFramesPerTick, drawnThisTick);
        }

        openTick = tick;
        drawnThisTick = 0;
    }

    void FramePacingStats::onFrameDrawn(antwika::time::Tick tick)
    {
        closeTick(tick);

        const auto nowTime = clock.getCurrentTime();

        if (!windowStart.has_value())
        {
            windowStart = nowTime;
        }

        if (lastDraw.has_value())
        {
            const auto gap =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    nowTime - *lastDraw);

            ++currentReport.intervals[bucketFor(gap)];
        }

        lastDraw = nowTime;
        dropRun = 0;
        ++drawnThisTick;
        ++currentReport.drawnFrames;

        const auto elapsedTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                nowTime - *windowStart);

        if (elapsedTime < window)
        {
            return;
        }

        closeWindow();
        windowStart = nowTime;
    }

    void FramePacingStats::onFrameDropped(antwika::time::Tick tick)
    {
        closeTick(tick);

        ++dropRun;
        ++currentReport.droppedFrames;

        currentReport.longestDropRun =
            std::max(currentReport.longestDropRun, dropRun);
    }

}
