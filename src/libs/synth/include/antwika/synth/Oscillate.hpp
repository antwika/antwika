#pragma once

#include <cstdint>

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/Waveshape.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    [[nodiscard]] float oscillate(
        Waveshape shape,
        double phase,
        std::uint64_t seed,
        FrameCount position) noexcept;

}
