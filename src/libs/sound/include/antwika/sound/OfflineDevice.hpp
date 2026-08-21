#pragma once

#include "antwika/sound/DeviceSpec.hpp"
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
        OfflineDevice(
            const DeviceSpec &spec, Waveform &destinationWaveform);

        OfflineDevice(const OfflineDevice &) = delete;
        OfflineDevice(OfflineDevice &&) = delete;

        OfflineDevice &operator=(const OfflineDevice &) = delete;
        OfflineDevice &operator=(OfflineDevice &&) = delete;

        void start(IRenderCallback &sourceCallback) override;
        void stop() override;
        [[nodiscard]] FrameCount advance(FrameCount frames) override;

        [[nodiscard]] WaveFormat format() const override;
        [[nodiscard]] FrameCount bufferFrames() const override;
        [[nodiscard]] FrameIndex framesPlayed() const override;

    private:
        WaveFormat wave;
        FrameCount bufferCount;
        Waveform &destinationWaveform;
        IRenderCallback *callback = nullptr;
        FrameIndex playedIndex = 0;
    };

}
