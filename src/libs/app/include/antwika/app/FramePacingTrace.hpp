#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <antwika/log/ILogger.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/IFramePacingSink.hpp"

namespace antwika::app
{

    inline constexpr std::size_t kPacingBuckets = 8;

    struct FramePacingReport final
    {
        std::uint32_t drawn = 0;

        std::uint32_t dropped = 0;

        std::uint32_t ticks = 0;

        std::uint32_t longestDropRun = 0;

        std::uint32_t leanestTick = 0;

        std::uint32_t fattestTick = 0;

        std::array<std::uint32_t, kPacingBuckets> intervals{};

        [[nodiscard]] bool operator==(
            const FramePacingReport &other) const = default;
    };

    [[nodiscard]] std::vector<std::string> pacingLines(
        const FramePacingReport &report);

    class FramePacingTrace final : public IFramePacingSink
    {
    public:
        FramePacingTrace(
            const antwika::time::IClock &clock,
            antwika::log::ILogger &logger,
            std::chrono::milliseconds window) noexcept;

        FramePacingTrace(const FramePacingTrace &) = delete;
        FramePacingTrace(FramePacingTrace &&) = delete;

        FramePacingTrace &operator=(const FramePacingTrace &) = delete;
        FramePacingTrace &operator=(FramePacingTrace &&) = delete;

        void drew(antwika::time::Tick tick) override;

        void dropped(antwika::time::Tick tick) override;

    private:
        void closeTick(antwika::time::Tick tick);

        void closeWindow();

        const antwika::time::IClock &clock;
        antwika::log::ILogger &logger;
        std::chrono::milliseconds window;
        std::optional<std::chrono::time_point<std::chrono::system_clock>>
            windowStart;
        std::optional<std::chrono::time_point<std::chrono::system_clock>>
            lastDraw;
        std::optional<antwika::time::Tick> openTick;
        std::uint32_t drawnThisTick = 0;
        std::uint32_t dropRun = 0;
        FramePacingReport gathering;
    };

}
