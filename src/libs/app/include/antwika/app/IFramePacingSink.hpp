#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    class IFramePacingSink
    {
    public:
        virtual ~IFramePacingSink() = default;

        virtual void drew(antwika::time::Tick tick) = 0;

        virtual void dropped(antwika::time::Tick tick) = 0;
    };

}
