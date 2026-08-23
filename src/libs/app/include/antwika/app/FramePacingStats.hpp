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

#include "antwika/app/FramePacingReport.hpp"
#include "antwika/app/IFramePacingSink.hpp"

namespace antwika::app
{

    [[nodiscard]] std::vector<std::string> getFormatPacingReport(
        const FramePacingReport &report);

    class FramePacingStats final : public IFramePacingSink
    {
    public:
        FramePacingStats(
            const antwika::time::IClock &clock,
            antwika::log::ILogger &logger,
            std::chrono::milliseconds window) noexcept;

        FramePacingStats(const FramePacingStats &) = delete;
        FramePacingStats(FramePacingStats &&) = delete;

        FramePacingStats &operator=(const FramePacingStats &) = delete;
        FramePacingStats &operator=(FramePacingStats &&) = delete;

        void onFrameDrawn(antwika::time::Tick tick) override;

        void onFrameDropped(antwika::time::Tick tick) override;

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
        FramePacingReport currentReport;
    };

}
