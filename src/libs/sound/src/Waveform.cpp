#include "antwika/sound/Waveform.hpp"

#include <cstddef>

namespace antwika::sound
{

    FrameCount Waveform::frameCount() const noexcept
    {
        if (format.channels == 0)
        {
            return 0;
        }

        return samples.size() / format.channels;
    }

    bool Waveform::isValid() const noexcept
    {
        if (!format.isValid())
        {
            return false;
        }

        return samples.size() % format.channels == 0;
    }

}
