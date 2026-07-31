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

    /** @brief How many sounds a mixer plays at once unless told otherwise. */
    inline constexpr std::size_t kDefaultVoices = 32;

    /**
     * @brief How a mixer is set up.
     */
    struct MixerDesc
    {
        WaveFormat format;
        std::size_t maxVoices = kDefaultVoices;
    };

    /**
     * @brief Plays waveforms, and sums them into a device's buffer.
     *
     * **Nothing here allocates while rendering.** The voice pool is
     * sized in the constructor and never resized, which is what lets
     * render() stay noexcept and lets the same code serve a real-time
     * device later without being rewritten.
     *
     * A request whose waveform is at a different rate is refused rather
     * than resampled: this library does not resample, and quietly
     * playing something at the wrong speed is worse than saying so.
     */
    class Mixer final : public IRenderCallback
    {
    public:
        /**
         * @brief Construct a mixer over what it may play.
         * @param library Owns every waveform; **must outlive this**.
         * @param desc The format to mix at, and how many voices to hold.
         * @throws SoundError If the format is not one audio could be
         * described by, or no voices were asked for.
         */
        Mixer(const WaveformLibrary &library, const MixerDesc &desc);

        Mixer(const Mixer &) = delete;
        Mixer(Mixer &&) = delete;

        Mixer &operator=(const Mixer &) = delete;
        Mixer &operator=(Mixer &&) = delete;

        /**
         * @brief Ask for a sound at a moment.
         *
         * Resolving the id happens here rather than in render(), so
         * nothing on the render path can fail or look anything up.
         *
         * @param request What to play, and when.
         * @throws SoundError If no waveform has that id, or its rate is
         * not the rate this mixer runs at.
         */
        void play(const PlayRequest &request);

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
         * @brief Sum every sounding voice into a buffer.
         * @param out Where to write; overwritten rather than added to.
         * @param firstFrame The absolute index of out's first frame.
         */
        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override;

    private:
        struct Voice
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

        // Which voice is stolen when every one is busy.
        // Round-robin from where the last steal landed.
        // A burst of requests therefore spreads across the pool.
        std::size_t nextSteal = 0;
    };

} // namespace antwika::sound
