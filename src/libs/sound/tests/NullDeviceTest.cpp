#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "antwika/sound/NullDevice.hpp"
#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/Frames.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/mocks/MockRenderCallback.hpp"

using antwika::sound::DeviceSpec;
using antwika::sound::FrameCount;
using antwika::sound::NullDevice;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::mocks::MockRenderCallback;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    MATCHER_P(HasFrames, frames, "")
    {
        return arg.frames == frames && arg.isValid();
    }

    [[nodiscard]] DeviceSpec getUsable()
    {
        return DeviceSpec{
            .format = WaveFormat{.rate = 48000, .channels = 2},
            .preferredBufferFrames = 64};
    }
}

TEST(NullDeviceTest, Format_ReportsWhatItWasOpenedAs)
{
    const NullDevice device(getUsable());

    EXPECT_EQ(device.getFormat(), (WaveFormat{.rate = 48000, .channels = 2}));
    EXPECT_EQ(device.getBufferFrames(), 64U);
}

TEST(NullDeviceTest, BufferFrames_FallBackWhenNoneWasAsked)
{
    auto spec = getUsable();
    spec.preferredBufferFrames = 0;

    const NullDevice device(spec);

    EXPECT_EQ(device.getBufferFrames(), antwika::sound::kDefaultBufferFrames);
}

TEST(NullDeviceTest, Start_RefusesASecondStart)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);

    EXPECT_THROW(device.start(callback), SoundError);
}

TEST(NullDeviceTest, Start_IsAllowedAgainAfterAStop)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);
    device.stop();

    EXPECT_NO_THROW(device.start(callback));
}

TEST(NullDeviceTest, Advance_RendersNothingBeforeAStart)
{
    NullDevice device(getUsable());

    EXPECT_EQ(device.advance(128), 0U);
    EXPECT_EQ(device.getFramesPlayed(), 0U);
}

TEST(NullDeviceTest, Advance_RendersExactlyWhatWasAsked)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    EXPECT_CALL(callback, render(_, _)).Times(3);

    device.start(callback);

    EXPECT_EQ(device.advance(192), 192U);
    EXPECT_EQ(device.getFramesPlayed(), 192U);
}

TEST(NullDeviceTest, Advance_ClampsTheFinalChunkToWhatIsLeft)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    {
        ::testing::InSequence order;
        EXPECT_CALL(callback, render(HasFrames(FrameCount{64}), 0U));
        EXPECT_CALL(callback, render(HasFrames(FrameCount{6}), 64U));
    }

    device.start(callback);

    EXPECT_EQ(device.advance(70), 70U);
    EXPECT_EQ(device.getFramesPlayed(), 70U);
}

TEST(NullDeviceTest, FramesPlayed_MovesOnlyUnderAnAdvance)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);

    EXPECT_EQ(device.getFramesPlayed(), 0U);
    EXPECT_EQ(device.getFramesPlayed(), 0U);

    (void)device.advance(10);

    EXPECT_EQ(device.getFramesPlayed(), 10U);
}

TEST(NullDeviceTest, Advance_RendersNothingAfterAStop)
{
    NullDevice device(getUsable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);
    device.stop();

    EXPECT_EQ(device.advance(128), 0U);
}
