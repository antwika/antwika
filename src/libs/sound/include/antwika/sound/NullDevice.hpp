#pragma once

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    /**
     * @brief A device that renders and then discards.
     *
     * **It does not advance a clock of its own**: framesPlayed() moves
     * only under pump(), which is the same standing gfx::NullBackend
     * has and is what makes a headless run instantaneous rather than
     * real-time.
     *
     * It still calls the callback, and still hands it ascending
     * contiguous absolute frame indices, so a mixer is exercised exactly
     * as a real device would exercise it.
     */
    class NullDevice final : public IDevice
    {
    public:
        /**
         * @brief Open a device that will discard what it renders.
         * @param desc What to open it as; already validated by whatever
         * constructed this.
         */
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

} // namespace antwika::sound
