#pragma once

#include <cstddef>
#include <vector>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/PlayRequest.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/WaveformLibrary.hpp"

namespace antwika::sound
{

    inline constexpr std::size_t kDefaultVoices = 32;

    struct MixerDesc final
    {
        WaveFormat format;
        std::size_t maxVoices = kDefaultVoices;
    };

    class Mixer final : public IRenderCallback
    {
    public:
        Mixer(const WaveformLibrary &library, const MixerDesc &desc);

        Mixer(const Mixer &) = delete;
        Mixer(Mixer &&) = delete;

        Mixer &operator=(const Mixer &) = delete;
        Mixer &operator=(Mixer &&) = delete;

        void play(const PlayRequest &request);

        void stopAll() noexcept;

        [[nodiscard]] std::size_t activeVoices() const noexcept;

        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override;

    private:
        struct Voice final
        {
            const Waveform *source = nullptr;
            FrameIndex startFrame = 0;
            FrameCount cursor = 0;
            float left = 1.0F;
            float right = 1.0F;
            bool looping = false;
            bool active = false;
        };

        const WaveformLibrary &library;
        WaveFormat wave;
        std::vector<Voice> voices;

        std::size_t nextSteal = 0;
    };

}
