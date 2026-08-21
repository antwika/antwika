#pragma once

#include <array>
#include <cstdint>
#include <antwika/log/ILogger.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>
#include "antwika/app/IFramePacingSink.hpp"

namespace antwika::app
{

    inline constexpr std::size_t kPacingBuckets = 8;

    struct FramePacingReport final
    {
        std::uint32_t drawnFrames = 0;

        std::uint32_t droppedFrames = 0;

        std::uint32_t ticks = 0;

        std::uint32_t longestDropRun = 0;

        std::uint32_t minFramesPerTick = 0;

        std::uint32_t maxFramesPerTick = 0;

        std::array<std::uint32_t, kPacingBuckets> intervals{};

        [[nodiscard]] bool operator==(
            const FramePacingReport &other) const = default;
    };

}
