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

        constexpr std::size_t kChunkFrames = 4096;

        struct PlacedNote final
        {
            sound::FrameIndex start = 0;
            sound::FrameCount frames = 0;
            pattern::Controls value{};
        };

        [[nodiscard]] std::int64_t framesAt(
            const Cycle &at, const std::int64_t frames) noexcept
        {
            return at.numerator() * frames / at.denominator();
        }
    }

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
            const auto pace = desc.framesPerCycle / speed;

            frames = pace.numerator() / pace.denominator();

            if (frames > 0)
            {
                for (const auto &hap : wave.playing.queryAll(
                         Span{Cycle(0), Cycle(1)}))
                {
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
        catch (const pattern::PatternError &) // GCOVR_EXCL_LINE
        {
            notes.clear();
            frames = 0;
        }

        if (frames <= 0)
        {
            return image;
        }

        const sound::WaveFormat format{
            .rate = desc.rate, .channels = sound::kStereo};

        synth::SynthMixer mixer(synth::SynthMixerDesc{.format = format});

        for (const auto &note : notes)
        {
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
    } // GCOVR_EXCL_LINE

}
