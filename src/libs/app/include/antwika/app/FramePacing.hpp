#pragma once

#include <chrono>
#include <cstdint>
#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/IFramePump.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>
#include "antwika/app/IFramePacingSink.hpp"
#include "antwika/app/IFramePass.hpp"

namespace antwika::app
{

    struct FramePacing final
    {
        std::chrono::milliseconds tickInterval{0};

        std::uint32_t framesPerTick = 1;
    };

}
