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

    inline constexpr std::size_t kDefaultVoices = 32;

    struct SynthMixerDesc final
    {
        WaveFormat format;
        std::size_t maxVoices = kDefaultVoices;
    };

    class SynthMixer final : public IRenderCallback
    {
    public:
        explicit SynthMixer(const SynthMixerDesc &desc);

        SynthMixer(const SynthMixer &) = delete;
        SynthMixer(SynthMixer &&) = delete;

        SynthMixer &operator=(const SynthMixer &) = delete;
        SynthMixer &operator=(SynthMixer &&) = delete;

        void trigger(const TriggerRequest &request);

        void stopAll() noexcept;

        [[nodiscard]] std::size_t activeVoices() const noexcept;

        [[nodiscard]] WaveFormat format() const noexcept;

        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override;

    private:
        struct Voice final
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

        std::size_t nextSteal = 0;
    };

}
