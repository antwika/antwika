#pragma once

#include <raylib.h>

#include <cstddef>
#include <memory>
#include <vector>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "RaylibAudioRuntime.hpp"

namespace antwika::raylib
{

    using antwika::sound::DeviceDesc;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;
    using antwika::sound::IDevice;
    using antwika::sound::IRenderCallback;
    using antwika::sound::WaveFormat;

    class RaylibDevice final : public IDevice
    {
    public:
        RaylibDevice(ILogger &logger, const DeviceDesc &desc);

        RaylibDevice(const RaylibDevice &) = delete;
        RaylibDevice(RaylibDevice &&) = delete;

        RaylibDevice &operator=(const RaylibDevice &) = delete;
        RaylibDevice &operator=(RaylibDevice &&) = delete;

        ~RaylibDevice() override;

        void start(IRenderCallback &callback) override;

        void stop() override;

        [[nodiscard]] FrameCount pump(FrameCount frames) override;

        [[nodiscard]] WaveFormat format() const override;

        [[nodiscard]] FrameCount bufferFrames() const override;

        [[nodiscard]] FrameIndex framesPlayed() const override;

    private:
        void render(FrameCount frames);

        void drain();

        [[nodiscard]] FrameCount pendingFrames() const noexcept;

        std::shared_ptr<RaylibAudioRuntime> audio;

        AudioStream stream{};
        bool streaming = false;

        WaveFormat wave;
        FrameCount buffer = 0;

        IRenderCallback *sink = nullptr;

        FrameIndex rendered = 0;
        FrameIndex accepted = 0;

        mutable FrameIndex played = 0;

        std::vector<std::vector<float>> planes;
        std::vector<float> pending;
        std::size_t pendingRead = 0;
    };

}
