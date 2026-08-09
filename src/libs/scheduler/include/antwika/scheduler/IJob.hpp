#pragma once

#include <antwika/time/Tick.hpp>

namespace antwika::scheduler
{

    class IJob
    {
    public:
        virtual ~IJob() = default;

        virtual void execute(antwika::time::Tick tick) = 0;
    };

}
