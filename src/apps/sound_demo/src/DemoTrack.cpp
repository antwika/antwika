#include "antwika/sound_demo/DemoTrack.hpp"

#include <cmath>
#include <cstddef>

namespace antwika::sound_demo
{

    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        // Loud enough to hear.
        // Quiet enough that eight at once do not sum past full scale.
        constexpr float kAmplitude = 0.2F;

        // A pan per note, so a listener can tell them apart.
        // It walks left to right across the eight of them.
        [[nodiscard]] float panFor(std::size_t note)
        {
            const auto steps = static_cast<float>(kNoteCount - 1);
            return -1.0F + 2.0F * static_cast<float>(note) / steps;
        }
    } // namespace

    Waveform demoTone(
        const WaveFormat &format, double hertz, FrameCount frames)
    {
        Waveform tone;
        tone.format = format;
        tone.samples.reserve(
            static_cast<std::size_t>(frames) * format.channels);

        for (FrameCount frame = 0; frame < frames; ++frame)
        {
            // A straight fade out, so a note ends rather than stops.
            // An abrupt end is a click.
            // That is the one artefact a sound demo must not open with.
            //
            // Counting the frame written lands the last on silence.
            const auto position = static_cast<double>(frame + 1)
                / static_cast<double>(frames);

            const auto envelope = 1.0 - position;

            const auto angle = 2.0 * kPi * hertz
                * static_cast<double>(frame) / static_cast<double>(format.rate);

            const auto value = static_cast<float>(
                static_cast<double>(kAmplitude) * envelope * std::sin(angle));

            for (std::size_t channel = 0; channel < format.channels;
                 ++channel)
            {
                tone.samples.push_back(value);
            }
        }

        return tone;

        // The excluded line is the local tone's unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

    std::vector<PlayRequest> demoSchedule(
        WaveformId waveform, FrameCount spacing)
    {
        std::vector<PlayRequest> notes;
        notes.reserve(kNoteCount);

        for (std::size_t note = 0; note < kNoteCount; ++note)
        {
            notes.push_back(
                PlayRequest{
                    .waveform = waveform,
                    .startFrame = note * spacing,
                    .gain = 1.0F,
                    .pan = panFor(note),
                    .looping = false});
        }

        return notes;

        // The excluded line is the local notes' unwind destructor.
        // Nothing between its construction and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::sound_demo
