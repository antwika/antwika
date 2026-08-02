#pragma once

#include <stdexcept>

namespace antwika::synth
{

    /**
     * @brief Thrown when a voice could not be built from a description.
     *
     * Every cause is a description that names something no oscillator
     * could produce: a voice lasting no frames, a periodic shape with no
     * frequency, a sustain outside its range, or a filter cutoff at or
     * above half the rate it would run at.
     *
     * **Nothing on the render path raises this**, by construction rather
     * than by care, in exactly the way antwika::sound::Mixer's render()
     * has no error path.
     * A voice that exists was checked when it was triggered, so what is
     * left to render is arithmetic over numbers already known good.
     *
     * Deliberately a single, specific, catchable type, mirroring
     * antwika::sound::SoundError, which this library sits above.
     */
    class SynthError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::synth
