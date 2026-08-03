#pragma once

#include <cstddef>
#include <vector>

#include <antwika/sequencer/Rational.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "antwika/music_editor/Score.hpp"

namespace antwika::music_editor
{

    /**
     * @brief How many columns one cycle's audio is folded into.
     *
     * Enough that a band a pane wide reads smoothly, and few enough
     * that an image is a couple of kilobytes; the scene samples the
     * nearest column per pixel, so the band's own width never matters
     * here.
     */
    inline constexpr std::size_t kWaveImageColumns = 512;

    /**
     * @brief One cycle of a voice's audio, folded to column extremes.
     *
     * What the waveform picture is drawn from: per column, the lowest
     * and highest sample the mix reached inside it, clamped to the
     * unit range.  A silent column holds zero for both, which the
     * scene draws as the flat midline it is.
     */
    struct WaveImage
    {
        /** @brief Per column, the lowest sample reached, in [-1, 0]. */
        std::vector<float> low{};

        /** @brief Per column, the highest reached, in [0, 1]. */
        std::vector<float> high{};

        /**
         * @brief Compare two images.
         * @param other The image to compare against.
         * @return True when both columns match sample for sample.
         */
        [[nodiscard]] bool operator==(const WaveImage &other) const
            = default;
    };

    /**
     * @brief What an offline render runs at.
     *
     * The same two numbers the live playback was built over, handed
     * in rather than read, so the picture is of the sound the run
     * actually makes.
     */
    struct WaveRenderDesc
    {
        /** @brief The rate the voices are generated at. */
        sound::SampleRate rate = sound::kDefaultSampleRate;

        /** @brief How long one cycle lasts at normal speed. */
        sequencer::Rational framesPerCycle{};

        /**
         * @brief Compare two descs.
         * @param other The desc to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(const WaveRenderDesc &other) const
            = default;
    };

    /**
     * @brief Render one cycle of one voice's actual audio.
     *
     * The real thing rather than a sketch of it: every note the line's
     * pattern puts in cycle nought is sounded through soundNote() --
     * harmony and echo included -- into a private synth mixer, exactly
     * as the live playback sounds it, and the generated samples are
     * folded to per-column extremes.
     *
     * A pattern that parses and still refuses its window comes out as
     * silence, exactly as the line falls silent; so does a pace that
     * leaves a cycle no frames at all.
     *
     * @param wave The line's pattern and sound, as Score reads them.
     * @param desc The rate and the cycle length at normal speed.
     * @param speed The multiplier the run is set to: 2 is twice as
     * fast, and half the frames to a cycle.
     * @param columns How many columns to fold the cycle into.
     * @return The image, sized to columns whatever happened.
     */
    [[nodiscard]] WaveImage renderWaveImage(
        const Waveform &wave,
        const WaveRenderDesc &desc,
        sequencer::Rational speed,
        std::size_t columns);

} // namespace antwika::music_editor
