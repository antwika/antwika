#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    struct Waveform final
    {
        WaveFormat format;

        std::vector<float> samples;

        [[nodiscard]] FrameCount frameCount() const noexcept;

        [[nodiscard]] bool isValid() const noexcept;

        [[nodiscard]] bool operator==(const Waveform &other) const = default;
    };

}
