#include "antwika/sound/NullDevice.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"
#include "antwika/sound/mocks/MockRenderCallback.hpp"

using antwika::sound::DeviceDesc;
using antwika::sound::NullDevice;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::mocks::MockRenderCallback;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] DeviceDesc usable()
    {
        return DeviceDesc{
            .format = WaveFormat{.rate = 48000, .channels = 2},
            .preferredBufferFrames = 64};
    }
} // namespace

TEST(NullDeviceTest, ReportsWhatItWasOpenedAs)
{
    const NullDevice device(usable());

    EXPECT_EQ(device.format(), (WaveFormat{.rate = 48000, .channels = 2}));
    EXPECT_EQ(device.bufferFrames(), 64U);
}

// A zero buffer would render nothing for ever.
// So it falls back rather than being taken at its word.
TEST(NullDeviceTest, FallsBackWhenNoBufferSizeWasAskedFor)
{
    auto desc = usable();
    desc.preferredBufferFrames = 0;

    const NullDevice device(desc);

    EXPECT_EQ(device.bufferFrames(), antwika::sound::kDefaultBufferFrames);
}

TEST(NullDeviceTest, Start_RefusesASecondStart)
{
    NullDevice device(usable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);

    EXPECT_THROW(device.start(callback), SoundError);
}

TEST(NullDeviceTest, Start_IsAllowedAgainAfterAStop)
{
    NullDevice device(usable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);
    device.stop();

    EXPECT_NO_THROW(device.start(callback));
}

TEST(NullDeviceTest, Pump_RendersNothingBeforeAStart)
{
    NullDevice device(usable());

    EXPECT_EQ(device.pump(128), 0U);
    EXPECT_EQ(device.framesPlayed(), 0U);
}

TEST(NullDeviceTest, Pump_RendersExactlyWhatWasAsked)
{
    NullDevice device(usable());
    NiceMock<MockRenderCallback> callback;

    EXPECT_CALL(callback, render(_, _)).Times(3);

    device.start(callback);

    EXPECT_EQ(device.pump(192), 192U);
    EXPECT_EQ(device.framesPlayed(), 192U);
}

// Nothing advances on its own.
// That is what makes a headless run instantaneous rather than real-time.
TEST(NullDeviceTest, FramesPlayed_MovesOnlyUnderAPump)
{
    NullDevice device(usable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);

    EXPECT_EQ(device.framesPlayed(), 0U);
    EXPECT_EQ(device.framesPlayed(), 0U);

    (void)device.pump(10);

    EXPECT_EQ(device.framesPlayed(), 10U);
}

TEST(NullDeviceTest, Pump_RendersNothingAfterAStop)
{
    NullDevice device(usable());
    NiceMock<MockRenderCallback> callback;

    device.start(callback);
    device.stop();

    EXPECT_EQ(device.pump(128), 0U);
}
