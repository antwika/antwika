#pragma once

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IDevice.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"

namespace antwika::sound
{

    /**
     * @brief A device that renders into a waveform instead of a speaker.
     *
     * **Not a backend**, and it lives here rather than under backends/
     * because it names no framework: it is what lets a mixer's output be
     * asserted sample by sample with no hardware, no thread and no file.
     *
     * It appends, so pumping twice produces one continuous waveform
     * rather than two, which is what makes a chunk size something a test
     * can vary without changing the answer.
     */
    class OfflineDevice final : public IDevice
    {
    public:
        /**
         * @brief Open a device that writes into a waveform.
         * @param desc What to render as; the sink takes this format.
         * @param sink Where rendered frames are appended; **must outlive
         * this object**.
         */
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

} // namespace antwika::sound
