#pragma once

#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    class NullDevice final : public IDevice
    {
    public:
        explicit NullDevice(const DeviceSpec &spec);

        NullDevice(const NullDevice &) = delete;
        NullDevice(NullDevice &&) = delete;

        NullDevice &operator=(const NullDevice &) = delete;
        NullDevice &operator=(NullDevice &&) = delete;

        void start(IRenderCallback &callback) override;
        void stop() override;
        [[nodiscard]] FrameCount advance(FrameCount frames) override;

        [[nodiscard]] WaveFormat getFormat() const override;
        [[nodiscard]] FrameCount getBufferFrames() const override;
        [[nodiscard]] FrameIndex getFramesPlayed() const override;

    private:
        WaveFormat wave;
        FrameCount bufferCount;
        IRenderCallback *sinkCallback = nullptr;
        FrameIndex playedIndex = 0;
    };

}
