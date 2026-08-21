#pragma once

#include <antwika/time/FrameRate.hpp>

namespace antwika::editor
{

    struct FrameMeters final
    {
        time::FrameRate frameRate;

        time::FrameRate workRate;

        time::FrameRate lampRate;

        time::FrameRate sightRate;

        time::FrameRate worldRate;

        time::FrameRate uiRate;

        time::FrameRate seamRate;

        time::FrameRate hideRate;
    };

}
