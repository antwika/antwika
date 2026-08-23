#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/sound/NullSoundBackend.hpp"
#include "antwika/sound/DeviceSpec.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"

using antwika::log::mocks::MockLogger;
using antwika::sound::DeviceSpec;
using antwika::sound::NullSoundBackend;
using antwika::sound::SoundCapabilities;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using ::testing::NiceMock;

TEST(NullSoundBackendTest, Name_IsNull)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_EQ(backend.getName(), "null");
}

TEST(NullSoundBackendTest, Capabilities_ClaimNeitherPlaybackNorAThread)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_EQ(
        backend.getCapabilities(),
        (SoundCapabilities{.playback = false, .selfDriven = false}));
}

TEST(NullSoundBackendTest, OpenDevice_HandsBackADeviceAtTheFormatAsked)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    const auto device = backend.openDevice(
        DeviceSpec{.format = WaveFormat{.rate = 44100, .channels = 1}});

    ASSERT_NE(device, nullptr);
    EXPECT_EQ(device->getFormat(), (WaveFormat{.rate = 44100, .channels = 1}));
}

TEST(NullSoundBackendTest, OpenDevice_RefusesAFormatThatIsNotOne)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_THROW(
        (void)backend.openDevice(
            DeviceSpec{.format = WaveFormat{.rate = 0, .channels = 2}}),
        SoundError);
}
