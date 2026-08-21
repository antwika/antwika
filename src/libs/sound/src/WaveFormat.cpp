#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    bool WaveFormat::isValid() const noexcept
    {
        return rate > 0 && channels >= kMonoCount && channels <= kMaxChannels;
    }

}
