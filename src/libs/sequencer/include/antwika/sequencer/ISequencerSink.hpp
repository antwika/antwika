#pragma once

#include <antwika/pattern/Controls.hpp>
#include <antwika/sound/Frames.hpp>

namespace antwika::sequencer
{

    using antwika::pattern::Controls;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;

    /**
     * @brief Receives what a sequencer decided to sound.
     *
     * **This is the seam that keeps this library ignorant of audio.**
     * A sequencer says *what* begins and *when*, in the caller's own
     * controls; deciding that a control means a frequency, and that a
     * frequency means a voice, belongs to the application.
     * So nothing here depends on antwika::synth, and a sequencer driving
     * something that is not a synthesiser needs no change at all.
     *
     * **Three arguments rather than one struct**, so that handing an
     * event on copies nothing.
     * A struct would have to own its controls, which is an allocation
     * for every note on a path that runs for as long as the program
     * does.
     * A sink that wants to keep one copies it deliberately instead.
     */
    class ISequencerSink
    {
    public:
        virtual ~ISequencerSink() = default;

        /**
         * @brief Take one event that begins.
         * @param value What the pattern's event carried; borrowed only
         * for the duration of this call.
         * @param startFrame The absolute frame it begins on.
         * @param frames How many frames it lasts.
         */
        virtual void trigger(
            const Controls &value,
            FrameIndex startFrame,
            FrameCount frames)
            = 0;
    };

} // namespace antwika::sequencer
