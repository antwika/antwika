#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <antwika/sound/fakes/FakePerChannelCallback.hpp>
#include <antwika/sound/fakes/FakeRampCallback.hpp>

#include "antwika/sound/OfflineDevice.hpp"
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

using antwika::sound::fakes::FakePerChannelCallback;
using antwika::sound::fakes::FakeRampCallback;

namespace
{
    [[nodiscard]] DeviceDesc usable(antwika::sound::FrameCount buffer)
    {
        return DeviceDesc{
            .format = WaveFormat{.rate = 48000, .channels = 2},
            .preferredBufferFrames = buffer};
    }
}

TEST(OfflineDeviceTest, Ctor_StampsTheOutputWaveformWithTheFormat)
{
    Waveform out;
    const OfflineDevice device(usable(64), out);

    EXPECT_EQ(out.format, (WaveFormat{.rate = 48000, .channels = 2}));
}

TEST(OfflineDeviceTest, Pump_AppendsExactlyTheFramesAsked)
{
    Waveform out;
    OfflineDevice device(usable(64), out);
    FakeRampCallback callback;

    device.start(callback);

    EXPECT_EQ(device.pump(480), 480U);
    EXPECT_EQ(out.frameCount(), 480U);
    EXPECT_TRUE(out.isComplete());
}

TEST(OfflineDeviceTest, Pump_ProducesTheSameAudioWhateverTheBufferSize)
{
    Waveform small;
    Waveform large;

    {
        OfflineDevice device(usable(1), small);
        FakeRampCallback callback;
        device.start(callback);
        (void)device.pump(100);
    }

    {
        OfflineDevice device(usable(1024), large);
        FakeRampCallback callback;
        device.start(callback);
        (void)device.pump(100);
    }

    ASSERT_EQ(small.frameCount(), 100U);
    EXPECT_EQ(small, large);
}

TEST(OfflineDeviceTest, Pump_InterleavesEveryChannelInFrameOrder)
{
    Waveform out;
    OfflineDevice device(usable(2), out);
    FakePerChannelCallback callback;

    device.start(callback);

    EXPECT_EQ(device.pump(5), 5U);

    const std::vector<float> expected{
        0.0F, 100.0F, 1.0F, 101.0F, 2.0F, 102.0F,
        3.0F, 103.0F, 4.0F, 104.0F};

    EXPECT_EQ(out.samples, expected);
}

TEST(OfflineDeviceTest, Pump_KeepsAppendingAcrossCalls)
{
    Waveform out;
    OfflineDevice device(usable(8), out);
    FakeRampCallback callback;

    device.start(callback);
    (void)device.pump(8);
    (void)device.pump(8);

    ASSERT_EQ(out.frameCount(), 16U);

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
    FakeRampCallback callback;

    device.start(callback);

    EXPECT_THROW(device.start(callback), SoundError);
}

TEST(OfflineDeviceTest, Format_ReportsWhatItWasOpenedWith)
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
    FakeRampCallback callback;

    device.start(callback);
    (void)device.pump(8);
    device.stop();

    EXPECT_EQ(out.frameCount(), 8U);
    EXPECT_EQ(device.pump(8), 0U);
}
