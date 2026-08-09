#pragma once

#include "antwika/pattern/Hap.hpp"

namespace antwika::pattern
{

    class IHapSink
    {
    public:
        virtual ~IHapSink() = default;

        virtual void accept(const Hap &hap) = 0;
    };

}
