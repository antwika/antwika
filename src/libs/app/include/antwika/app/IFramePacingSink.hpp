#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    class IFramePacingSink
    {
    public:
        virtual ~IFramePacingSink() = default;

        virtual void onFrameDrawn(antwika::time::Tick tick) = 0;

        virtual void onFrameDropped(antwika::time::Tick tick) = 0;
    };

}
