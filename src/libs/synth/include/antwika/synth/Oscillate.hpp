#pragma once

#include <cstdint>

#include <antwika/sound/Frames.hpp>

#include "antwika/synth/Waveshape.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameCount;

    /**
     * @brief Sample one shape at one instant.
     *
     * Pure, so a sample can be asked for out of order and asserted on
     * without a voice being driven to it first.
     *
     * **Noise is positional rather than generated**, which is the
     * decision here worth defending.
     * A shared generator advanced once per sample would make each
     * voice's output depend on how many other voices were sounding and
     * on the order they were stolen in -- a divergence that appears only
     * under load and never in a test with one voice in it.
     * Hashing the position instead means a voice's noise is the same
     * whichever other voices exist, and the same on every run.
     *
     * @param shape Which shape to trace.
     * @param phase Where in the cycle, from zero up to but not including
     * one; ignored by Noise, which has no cycle.
     * @param seed What distinguishes one noise voice from another;
     * ignored by every periodic shape.
     * @param position How many frames the voice has sounded for; ignored
     * by every periodic shape.
     * @return The sample, from minus one to plus one, and zero for a
     * shape no enumerator names.
     */
    [[nodiscard]] float oscillate(
        Waveshape shape,
        double phase,
        std::uint64_t seed,
        FrameCount position) noexcept;

} // namespace antwika::synth
