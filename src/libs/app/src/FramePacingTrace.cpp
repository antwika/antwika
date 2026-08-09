#include "antwika/app/FramePacingTrace.hpp"

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

        [[nodiscard]] std::string counted(
            const char *named, std::uint32_t amount)
        {
            return std::string(named) + " " + std::to_string(amount);
        }
    }

    std::vector<std::string> pacingLines(const FramePacingReport &report)
    {
        const auto scheduled = report.drawn + report.dropped;

        std::string spread = "pacing gaps ";

        for (std::size_t at = 0; at < kPacingBuckets; ++at)
        {
            spread += std::string(kBucketNames[at]) + ":"
                      + std::to_string(report.intervals[at]) + " ";
        }

        std::vector<std::string> lines;

        lines.push_back(
            "pacing " + counted("drawn", report.drawn) + " of "
            + std::to_string(scheduled) + " over "
            + counted("ticks", report.ticks));

        lines.push_back(
            "pacing " + counted("longest drop run", report.longestDropRun)
            + ", " + counted("leanest tick", report.leanestTick)
            + ", " + counted("fattest tick", report.fattestTick));

        lines.push_back(std::move(spread));

        return lines;
    } // GCOVR_EXCL_LINE

    FramePacingTrace::FramePacingTrace(
        const antwika::time::IClock &clock,
        antwika::log::ILogger &logger,
        std::chrono::milliseconds window) noexcept
        : clock(clock), logger(logger), window(window)
    {
    }

    void FramePacingTrace::closeWindow()
    {
        for (const auto &line : pacingLines(gathering))
        {
            logger.log(antwika::log::Level::Info, line);
        }

        gathering = FramePacingReport{};
    }

    void FramePacingTrace::closeTick(antwika::time::Tick tick)
    {
        if (openTick.has_value() && *openTick == tick)
        {
            return;
        }

        if (openTick.has_value())
        {
            ++gathering.ticks;

            gathering.fattestTick =
                std::max(gathering.fattestTick, drawnThisTick);

            gathering.leanestTick = gathering.ticks == 1
                ? drawnThisTick
                : std::min(gathering.leanestTick, drawnThisTick);
        }

        openTick = tick;
        drawnThisTick = 0;
    }

    void FramePacingTrace::drew(antwika::time::Tick tick)
    {
        closeTick(tick);

        const auto now = clock.now();

        if (!windowStart.has_value())
        {
            windowStart = now;
        }

        if (lastDraw.has_value())
        {
            const auto gap =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    now - *lastDraw);

            ++gathering.intervals[bucketFor(gap)];
        }

        lastDraw = now;
        dropRun = 0;
        ++drawnThisTick;
        ++gathering.drawn;

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - *windowStart);

        if (elapsed < window)
        {
            return;
        }

        closeWindow();
        windowStart = now;
    }

    void FramePacingTrace::dropped(antwika::time::Tick tick)
    {
        closeTick(tick);

        ++dropRun;
        ++gathering.dropped;

        gathering.longestDropRun =
            std::max(gathering.longestDropRun, dropRun);
    }

}
