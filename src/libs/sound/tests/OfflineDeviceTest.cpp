#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include <antwika/sound/fakes/FakePerChannelCallback.hpp>
#include <antwika/sound/fakes/FakeRampCallback.hpp>

#include "antwika/sound/OfflineDevice.hpp"
#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/Waveform.hpp"

using antwika::sound::DeviceSpec;
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
    [[nodiscard]] DeviceSpec usable(antwika::sound::FrameCount bufferCount)
    {
        return DeviceSpec{
            .format = WaveFormat{.rate = 48000, .channels = 2},
            .preferredBufferFrames = bufferCount};
    }
}

TEST(OfflineDeviceTest, Ctor_StampsTheOutputWaveformWithTheFormat)
{
    Waveform waveform;
    const OfflineDevice device(usable(64), waveform);

    EXPECT_EQ(waveform.format, (WaveFormat{.rate = 48000, .channels = 2}));
}

TEST(OfflineDeviceTest, Advance_AppendsExactlyTheFramesAsked)
{
    Waveform waveform;
    OfflineDevice device(usable(64), waveform);
    FakeRampCallback callback;

    device.start(callback);

    EXPECT_EQ(device.advance(480), 480U);
    EXPECT_EQ(waveform.frameCount(), 480U);
    EXPECT_TRUE(waveform.isValid());
}

TEST(OfflineDeviceTest, Advance_ProducesTheSameAudioWhateverTheBufferSize)
{
    Waveform smallWaveform;
    Waveform largeWaveform;

    {
        OfflineDevice device(usable(1), smallWaveform);
        FakeRampCallback callback;
        device.start(callback);
        (void)device.advance(100);
    }

    {
        OfflineDevice device(usable(1024), largeWaveform);
        FakeRampCallback callback;
        device.start(callback);
        (void)device.advance(100);
    }

    ASSERT_EQ(smallWaveform.frameCount(), 100U);
    EXPECT_EQ(smallWaveform, largeWaveform);
}

TEST(OfflineDeviceTest, Advance_InterleavesEveryChannelInFrameOrder)
{
    Waveform waveform;
    OfflineDevice device(usable(2), waveform);
    FakePerChannelCallback callback;

    device.start(callback);

    EXPECT_EQ(device.advance(5), 5U);

    const std::vector<float> expectedSamples{
        0.0F, 100.0F, 1.0F, 101.0F, 2.0F, 102.0F,
        3.0F, 103.0F, 4.0F, 104.0F};

    EXPECT_EQ(waveform.samples, expectedSamples);
}

TEST(OfflineDeviceTest, Advance_KeepsAppendingAcrossCalls)
{
    Waveform waveform;
    OfflineDevice device(usable(8), waveform);
    FakeRampCallback callback;

    device.start(callback);
    (void)device.advance(8);
    (void)device.advance(8);

    ASSERT_EQ(waveform.frameCount(), 16U);

    EXPECT_EQ(waveform.samples[8 * 2], 8.0F);
}

TEST(OfflineDeviceTest, Advance_WritesNothingBeforeAStart)
{
    Waveform waveform;
    OfflineDevice device(usable(8), waveform);

    EXPECT_EQ(device.advance(8), 0U);
    EXPECT_TRUE(waveform.samples.empty());
}

TEST(OfflineDeviceTest, Start_RefusesASecondStart)
{
    Waveform waveform;
    OfflineDevice device(usable(8), waveform);
    FakeRampCallback callback;

    device.start(callback);

    EXPECT_THROW(device.start(callback), SoundError);
}

TEST(OfflineDeviceTest, Format_ReportsWhatItWasOpenedWith)
{
    Waveform waveform;
    OfflineDevice device(usable(0), waveform);

    EXPECT_EQ(device.format(), (WaveFormat{.rate = 48000, .channels = 2}));
    EXPECT_EQ(device.bufferFrames(), antwika::sound::kDefaultBufferFrames);
    EXPECT_EQ(device.framesPlayed(), 0U);
}

TEST(OfflineDeviceTest, Stop_LeavesWhatWasAlreadyRendered)
{
    Waveform waveform;
    OfflineDevice device(usable(8), waveform);
    FakeRampCallback callback;

    device.start(callback);
    (void)device.advance(8);
    device.stop();

    EXPECT_EQ(waveform.frameCount(), 8U);
    EXPECT_EQ(device.advance(8), 0U);
}
