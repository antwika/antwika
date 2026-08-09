#pragma once

#include <antwika/sound/Frames.hpp>

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    struct Adsr final
    {
        FrameCount attack = 0;

        FrameCount decay = 0;

        float sustain = 1.0F;

        FrameCount release = 0;

        [[nodiscard]] bool operator==(const Adsr &other) const = default;
    };

    [[nodiscard]] float envelopeAt(
        const Adsr &envelope, FrameCount since, FrameCount hold) noexcept;

}
