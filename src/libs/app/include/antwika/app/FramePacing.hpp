#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IFramePump.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/IFramePacingSink.hpp"
#include "antwika/app/IFramePass.hpp"

namespace antwika::app
{

    inline constexpr std::chrono::milliseconds kTickPeriod{16};

    inline constexpr std::uint32_t kTargetFps = 62;

    inline constexpr std::size_t kMaxCatchUpTicks = 5;

    inline constexpr gfx::Size kDefaultWindowSize{
        .width = 1280, .height = 720};

    struct FramePacing final
    {
        std::chrono::milliseconds tickInterval{0};

        std::uint32_t framesPerTick = 1;
    };

}
