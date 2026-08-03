#pragma once

#include <cstddef>
#include <vector>

#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "antwika/synth/Filter.hpp"
#include "antwika/synth/TriggerRequest.hpp"
#include "antwika/synth/VoiceDesc.hpp"

namespace antwika::synth
{

    using antwika::sound::FrameIndex;
    using antwika::sound::IRenderCallback;
    using antwika::sound::SampleBuffer;
    using antwika::sound::WaveFormat;

    /** @brief How many voices a mixer sounds at once unless told. */
    inline constexpr std::size_t kDefaultVoices = 32;

    /**
     * @brief How a synth mixer is set up.
     */
    struct SynthMixerDesc
    {
        WaveFormat format;
        std::size_t maxVoices = kDefaultVoices;
    };

    /**
     * @brief Generates voices, and sums them into a device's buffer.
     *
     * The counterpart of antwika::sound::Mixer, method for method, and
     * deliberately so: that one reads samples a library owns, this one
     * makes them up, and everything else about the two is the same.
     * Because it is an antwika::sound::IRenderCallback, every device,
     * backend and conformance test takes it unchanged, and
     * antwika::sound gains nothing at all.
     *
     * **Nothing here allocates while rendering.** The voice pool is
     * sized in the constructor and never resized, which is what lets
     * render() stay noexcept, and what would let the same code serve a
     * device that could not be pumped without being rewritten.
     *
     * Every refusal happens in trigger(), so by the time a voice is in
     * the pool there is nothing left for the render path to check.
     */
    class SynthMixer final : public IRenderCallback
    {
    public:
        /**
         * @brief Construct a mixer over how it will run.
         * @param desc The format to generate at, and how many voices.
         * @throws SynthError If the format is not one audio could be
         * described by, or no voices were asked for.
         */
        explicit SynthMixer(const SynthMixerDesc &desc);

        SynthMixer(const SynthMixer &) = delete;
        SynthMixer(SynthMixer &&) = delete;

        SynthMixer &operator=(const SynthMixer &) = delete;
        SynthMixer &operator=(SynthMixer &&) = delete;

        /**
         * @brief Ask for a voice at a moment.
         *
         * Checking and coefficient arithmetic both happen here rather
         * than in render(), so nothing on the render path can fail, look
         * anything up or call a transcendental.
         *
         * @param request What to sound, and when.
         * @throws SynthError If the voice would last no frames, if a
         * periodic shape names no usable frequency, if the sustain lies
         * outside zero to one, or if a filter is asked for a cutoff at or
         * above half the rate, or a resonance of zero or less.
         */
        void trigger(const TriggerRequest &request);

        /**
         * @brief Silence everything currently sounding.
         */
        void stopAll() noexcept;

        /**
         * @brief Get how many voices are currently sounding.
         * @return The count.
         */
        [[nodiscard]] std::size_t activeVoices() const noexcept;

        /**
         * @brief Get the format this mixer generates at.
         * @return The format, which is what a device must be opened as.
         */
        [[nodiscard]] WaveFormat format() const noexcept;

        /**
         * @brief Sum every sounding voice into a buffer.
         * @param out Where to write; overwritten rather than added to.
         * @param firstFrame The absolute index of out's first frame.
         */
        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override;

    private:
        struct Voice
        {
            VoiceDesc desc{};
            FilterCoefficients coefficients{};
            FilterState filter{};
            FrameIndex startFrame = 0;
            FrameCount elapsed = 0;
            double phase = 0.0;
            double vibratoPhase = 0.0;
            float left = 1.0F;
            float right = 1.0F;
            bool active = false;
        };

        [[nodiscard]] float nextSample(Voice &voice) const noexcept;

        WaveFormat wave;
        std::vector<Voice> voices;

        // Which voice is stolen when every one is busy.
        // Round-robin from where the last steal landed.
        // A burst of triggers therefore spreads across the pool.
        std::size_t nextSteal = 0;
    };

} // namespace antwika::synth
