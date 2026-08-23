#pragma once

#include <span>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    struct SampleBuffer final
    {
        std::span<const std::span<float>> channels;

        FrameCount frames = 0;

        [[nodiscard]] ChannelCount getChannelCount() const noexcept;

        [[nodiscard]] bool isValid() const noexcept;

        void silence() const noexcept;
    };

}
