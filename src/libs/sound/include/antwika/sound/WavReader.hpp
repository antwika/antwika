#pragma once

#include <iosfwd>

#include "antwika/sound/Waveform.hpp"

namespace antwika::sound
{

    class WavReader final
    {
    public:
        [[nodiscard]] Waveform read(std::istream &in) const;
    };

}
