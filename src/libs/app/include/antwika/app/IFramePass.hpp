#pragma once

#include <antwika/animation/Progress.hpp>

namespace antwika::app
{

    class IFramePass
    {
    public:
        virtual ~IFramePass() = default;

        virtual void draw(antwika::animation::Progress subTick) = 0;
    };

}
