#pragma once

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    class NullDevice final : public IDevice
    {
    public:
        explicit NullDevice(const DeviceDesc &desc);

        NullDevice(const NullDevice &) = delete;
        NullDevice(NullDevice &&) = delete;

        NullDevice &operator=(const NullDevice &) = delete;
        NullDevice &operator=(NullDevice &&) = delete;

        void start(IRenderCallback &callback) override;
        void stop() override;
        [[nodiscard]] FrameCount pump(FrameCount frames) override;

        [[nodiscard]] WaveFormat format() const override;
        [[nodiscard]] FrameCount bufferFrames() const override;
        [[nodiscard]] FrameIndex framesPlayed() const override;

    private:
        WaveFormat wave;
        FrameCount buffer;
        IRenderCallback *sink = nullptr;
        FrameIndex played = 0;
    };

}
