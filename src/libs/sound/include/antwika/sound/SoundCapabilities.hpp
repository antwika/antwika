#pragma once

namespace antwika::sound
{

    struct SoundCapabilities final
    {
        bool playback = false;

        bool selfDriven = false;

        [[nodiscard]] bool operator==(const SoundCapabilities &other) const
            = default;
    };

}
