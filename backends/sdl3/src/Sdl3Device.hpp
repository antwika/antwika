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

    class Sdl3Device final : public IDevice
    {
    public:
        Sdl3Device(ILogger &logger, const DeviceDesc &desc);

        Sdl3Device(const Sdl3Device &) = delete;
        Sdl3Device(Sdl3Device &&) = delete;

        Sdl3Device &operator=(const Sdl3Device &) = delete;
        Sdl3Device &operator=(Sdl3Device &&) = delete;

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

        mutable FrameIndex played = 0;

        std::vector<std::vector<float>> planes;
        std::vector<float> interleaved;
    };

}
