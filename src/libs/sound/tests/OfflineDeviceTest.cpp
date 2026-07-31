#include "antwika/sound/OfflineDevice.hpp"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"

using antwika::sound::DeviceDesc;
using antwika::sound::FrameIndex;
using antwika::sound::IRenderCallback;
using antwika::sound::OfflineDevice;
using antwika::sound::SampleBuffer;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::Waveform;

namespace
{
    // Writes the absolute frame index into every channel.
    // What came out then says exactly which frames it was.
    class Counting final : public IRenderCallback
    {
    public:
        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override
        {
            for (std::size_t frame = 0; frame < out.frames; ++frame)
            {
                for (const auto channel : out.channels)
                {
                    channel[frame] =
                        static_cast<float>(firstFrame + frame);
                }
            }
        }
    };

    [[nodiscard]] DeviceDesc usable(antwika::sound::FrameCount buffer)
    {
        return DeviceDesc{
            .format = WaveFormat{.rate = 48000, .channels = 2},
            .preferredBufferFrames = buffer};
    }
} // namespace

TEST(OfflineDeviceTest, TakesTheFormatItWasOpenedAt)
{
    Waveform out;
    const OfflineDevice device(usable(64), out);

    EXPECT_EQ(out.format, (WaveFormat{.rate = 48000, .channels = 2}));
}

TEST(OfflineDeviceTest, Pump_AppendsExactlyTheFramesAsked)
{
    Waveform out;
    OfflineDevice device(usable(64), out);
    Counting callback;

    device.start(callback);

    EXPECT_EQ(device.pump(480), 480U);
    EXPECT_EQ(out.frameCount(), 480U);
    EXPECT_TRUE(out.isComplete());
}

// The chunk size is the device's business, not the answer's.
// Pumping in ones and in thousands produces the same waveform.
TEST(OfflineDeviceTest, Pump_ProducesTheSameAudioWhateverTheBufferSize)
{
    Waveform small;
    Waveform large;

    {
        OfflineDevice device(usable(1), small);
        Counting callback;
        device.start(callback);
        (void)device.pump(100);
    }

    {
        OfflineDevice device(usable(1024), large);
        Counting callback;
        device.start(callback);
        (void)device.pump(100);
    }

    EXPECT_EQ(small, large);
}

TEST(OfflineDeviceTest, Pump_KeepsAppendingAcrossCalls)
{
    Waveform out;
    OfflineDevice device(usable(8), out);
    Counting callback;

    device.start(callback);
    (void)device.pump(8);
    (void)device.pump(8);

    ASSERT_EQ(out.frameCount(), 16U);

    // Frame eight really is the ninth frame rather than a second first.
    EXPECT_EQ(out.samples[8 * 2], 8.0F);
}

TEST(OfflineDeviceTest, Pump_WritesNothingBeforeAStart)
{
    Waveform out;
    OfflineDevice device(usable(8), out);

    EXPECT_EQ(device.pump(8), 0U);
    EXPECT_TRUE(out.samples.empty());
}

TEST(OfflineDeviceTest, Start_RefusesASecondStart)
{
    Waveform out;
    OfflineDevice device(usable(8), out);
    Counting callback;

    device.start(callback);

    EXPECT_THROW(device.start(callback), SoundError);
}

TEST(OfflineDeviceTest, ReportsWhatItWasOpenedWith)
{
    Waveform out;
    OfflineDevice device(usable(0), out);

    EXPECT_EQ(device.format(), (WaveFormat{.rate = 48000, .channels = 2}));
    EXPECT_EQ(device.bufferFrames(), antwika::sound::kDefaultBufferFrames);
    EXPECT_EQ(device.framesPlayed(), 0U);
}

TEST(OfflineDeviceTest, Stop_LeavesWhatWasAlreadyRendered)
{
    Waveform out;
    OfflineDevice device(usable(8), out);
    Counting callback;

    device.start(callback);
    (void)device.pump(8);
    device.stop();

    EXPECT_EQ(out.frameCount(), 8U);
    EXPECT_EQ(device.pump(8), 0U);
}
