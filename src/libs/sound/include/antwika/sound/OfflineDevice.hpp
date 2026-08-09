#pragma once

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"

namespace antwika::sound
{

    class OfflineDevice final : public IDevice
    {
    public:
        OfflineDevice(const DeviceDesc &desc, Waveform &sink);

        OfflineDevice(const OfflineDevice &) = delete;
        OfflineDevice(OfflineDevice &&) = delete;

        OfflineDevice &operator=(const OfflineDevice &) = delete;
        OfflineDevice &operator=(OfflineDevice &&) = delete;

        void start(IRenderCallback &callback) override;
        void stop() override;
        [[nodiscard]] FrameCount pump(FrameCount frames) override;

        [[nodiscard]] WaveFormat format() const override;
        [[nodiscard]] FrameCount bufferFrames() const override;
        [[nodiscard]] FrameIndex framesPlayed() const override;

    private:
        WaveFormat wave;
        FrameCount buffer;
        Waveform &out;
        IRenderCallback *sink = nullptr;
        FrameIndex played = 0;
    };

}
