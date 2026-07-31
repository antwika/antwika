#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    bool WaveFormat::isValid() const noexcept
    {
        return rate > 0 && channels >= kMono && channels <= kMaxChannels;
    }

} // namespace antwika::sound
