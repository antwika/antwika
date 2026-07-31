#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "Sdl3Runtime.hpp"

namespace antwika::sdl3
{

    using antwika::sound::DeviceDesc;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;
    using antwika::sound::IDevice;
    using antwika::sound::IRenderCallback;
    using antwika::sound::WaveFormat;

    /**
     * @brief One SDL3 playback device, rendered by whoever pumps it.
     *
     * SDL_OpenAudioDeviceStream with a null callback is a **push**
     * model: SDL starts no thread of ours and calls nothing of ours, and
     * the caller hands buffers over with SDL_PutAudioStreamData.
     * So this device is pumped from the simulation thread exactly as
     * NullDevice is, and antwika::sound stays single-threaded.
     *
     * The consequence worth stating is that SDL owns the queue between
     * here and the hardware.  How far ahead a caller pushes is its own
     * pacing decision, and framesPlayed() is what it may read to make
     * one -- monotonic and advisory, never an input to what is computed.
     */
    class Sdl3Device final : public IDevice
    {
    public:
        /**
         * @brief Open a device.
         * @param logger Receives the device's diagnostics.
         * @param desc What to open it as.
         * @throws Sdl3Error If SDL refused to open it.
         */
        Sdl3Device(ILogger &logger, const DeviceDesc &desc);

        Sdl3Device(const Sdl3Device &) = delete;
        Sdl3Device(Sdl3Device &&) = delete;

        Sdl3Device &operator=(const Sdl3Device &) = delete;
        Sdl3Device &operator=(Sdl3Device &&) = delete;

        /**
         * @brief Close the device, stopping it first if it was started.
         */
        ~Sdl3Device() override;

        void start(IRenderCallback &callback) override;

        void stop() override;

        [[nodiscard]] FrameCount pump(FrameCount frames) override;

        [[nodiscard]] WaveFormat format() const override;

        [[nodiscard]] FrameCount bufferFrames() const override;

        [[nodiscard]] FrameIndex framesPlayed() const override;

    private:
        void render(FrameCount frames);

        Sdl3Subsystem audio;

        SDL_AudioStream *stream = nullptr;

        WaveFormat wave;
        FrameCount buffer = 0;

        IRenderCallback *sink = nullptr;
        FrameIndex pushed = 0;

        // The running maximum framesPlayed() has ever reported.
        // Mutable because reporting it is a read.
        // It keeps the answer monotonic without trusting SDL.
        mutable FrameIndex played = 0;

        // Sized once, in the constructor, and never resized.
        // render() therefore allocates nothing.
        // That is the promise antwika::sound makes for it.
        std::vector<std::vector<float>> planes;
        std::vector<float> interleaved;
    };

} // namespace antwika::sdl3
