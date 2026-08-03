#include "antwika/music_editor/WaveImage.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/pattern/Span.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/synth/SynthMixer.hpp>

#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    namespace
    {
        using antwika::pattern::Cycle;
        using antwika::pattern::Span;

        // How many frames are generated per render call.
        // A cycle is rendered offline, so the size is taste alone.
        constexpr std::size_t kChunkFrames = 4096;

        /**
         * @brief One note to sound, already placed in frames.
         *
         * Worked out of the exact rationals before any audio runs,
         * so the arithmetic that can refuse -- the pace division,
         * the query, a span's length -- is all inside one try.
         */
        struct PlacedNote
        {
            sound::FrameIndex start = 0;
            sound::FrameCount frames = 0;
            pattern::Controls value{};
        };

        /**
         * @brief Scale a cycle position into a frame count.
         * @param at The position or length, never negative.
         * @param frames What one whole cycle scales to.
         * @return The frames, rounded down.
         */
        [[nodiscard]] std::int64_t framesAt(
            const Cycle &at, const std::int64_t frames) noexcept
        {
            return at.numerator() * frames / at.denominator();
        }
    } // namespace

    WaveImage renderWaveImage(
        const Waveform &wave,
        const WaveRenderDesc &desc,
        const sequencer::Rational speed,
        const std::size_t columns)
    {
        WaveImage image;
        image.low.assign(columns, 0.0F);
        image.high.assign(columns, 0.0F);

        if (columns == 0)
        {
            return image;
        }

        std::int64_t frames = 0;
        std::vector<PlacedNote> notes;

        try
        {
            // Twice as fast is half the frames to a cycle.
            // The same arithmetic Playback::setSpeed runs.
            const auto pace = desc.framesPerCycle / speed;

            frames = pace.numerator() / pace.denominator();

            if (frames > 0)
            {
                for (const auto &hap : wave.playing.queryAll(
                         Span{Cycle(0), Cycle(1)}))
                {
                    // The whole says how long a note rings.
                    // The part says where the window first saw it.
                    const auto span = hap.whole.value_or(hap.part);

                    notes.push_back(PlacedNote{
                        .start = static_cast<sound::FrameIndex>(
                            framesAt(hap.part.begin(), frames)),
                        .frames = static_cast<sound::FrameCount>(
                            framesAt(span.length(), frames)),
                        .value = hap.value});
                }
            }
        }
        // The excluded line is the handler's no-match edge.
        // Only some other exception type would take it.
        // See docs/confirming-unreachable-branches.md.
        catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
        {
            // A pattern can parse and still refuse a window.
            // The picture is silence, exactly as the line is.
            notes.clear();
            frames = 0;
        }

        if (frames <= 0)
        {
            return image;
        }

        // A private pool, on the same format the live one runs.
        const sound::WaveFormat format{
            .rate = desc.rate, .channels = sound::kStereo};

        synth::SynthMixer mixer(synth::SynthMixerDesc{.format = format});

        for (const auto &note : notes)
        {
            // A refused note is silence here as it is live.
            // soundNote already demotes it, so the answer is spare.
            (void)soundNote(
                mixer, wave.preset, note.value, note.frames,
                note.start, 0);
        }

        std::vector<float> left(kChunkFrames);
        std::vector<float> right(kChunkFrames);

        std::array<std::span<float>, 2> channels{
            std::span<float>{left}, std::span<float>{right}};

        const auto total = static_cast<sound::FrameIndex>(frames);
        sound::FrameIndex done = 0;

        while (done < total)
        {
            const auto take = std::min<sound::FrameIndex>(
                kChunkFrames, total - done);

            mixer.render(
                sound::SampleBuffer{
                    .channels = channels, .frames = take},
                done);

            for (sound::FrameIndex at = 0; at < take; ++at)
            {
                const auto column = static_cast<std::size_t>(
                    (done + at) * columns / total);

                // The two channels folded to one.
                // Kept inside the range a band's height stands for.
                const auto mixed = (left[at] + right[at]) / 2.0F;
                const auto sample = std::clamp(mixed, -1.0F, 1.0F);

                image.low[column] =
                    std::min(image.low[column], sample);
                image.high[column] =
                    std::max(image.high[column], sample);
            }

            done += take;
        }

        return image;
        // Only an unwind destroys the locals at this brace.
        // Nothing between the last render and the return throws.
    } // GCOVR_EXCL_LINE

} // namespace antwika::music_editor
