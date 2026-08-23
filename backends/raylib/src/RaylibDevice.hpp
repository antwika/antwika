#pragma once

#include <raylib.h>

#include <cstddef>
#include <memory>
#include <vector>

#include <antwika/sound/DeviceSpec.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "RaylibAudioRuntime.hpp"

namespace antwika::raylib
{

    using antwika::sound::DeviceSpec;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;
    using antwika::sound::IDevice;
    using antwika::sound::IRenderCallback;
    using antwika::sound::WaveFormat;

    class RaylibDevice final : public IDevice
    {
    public:
        RaylibDevice(ILogger &logger, const DeviceSpec &spec);

        RaylibDevice(const RaylibDevice &) = delete;
        RaylibDevice(RaylibDevice &&) = delete;

        RaylibDevice &operator=(const RaylibDevice &) = delete;
        RaylibDevice &operator=(RaylibDevice &&) = delete;

        ~RaylibDevice() override;

        void start(IRenderCallback &callback) override;

        void stop() override;

        [[nodiscard]] FrameCount advance(FrameCount frames) override;

        [[nodiscard]] WaveFormat getFormat() const override;

        [[nodiscard]] FrameCount getBufferFrames() const override;

        [[nodiscard]] FrameIndex getFramesPlayed() const override;

    private:
        void render(FrameCount frames);

        void flushToStream();

        [[nodiscard]] FrameCount getPendingFrames() const noexcept;

        std::shared_ptr<RaylibAudioRuntime> audio;

        AudioStream stream{};
        bool streaming = false;

        WaveFormat wave;
        FrameCount bufferCount = 0;

        IRenderCallback *sinkCallback = nullptr;

        FrameIndex renderedIndex = 0;
        FrameIndex acceptedIndex = 0;

        mutable FrameIndex playedIndex = 0;

        std::vector<std::vector<float>> channelBuffers;
        std::vector<float> pending;
        std::size_t pendingRead = 0;
    };

}
