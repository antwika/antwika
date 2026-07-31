#include "antwika/sound/NullSoundBackend.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>

#include "antwika/sound/DeviceDesc.hpp"
#include "antwika/sound/SoundError.hpp"
#include "antwika/sound/WaveFormat.hpp"

using antwika::log::mocks::MockLogger;
using antwika::sound::DeviceDesc;
using antwika::sound::NullSoundBackend;
using antwika::sound::SoundCapabilities;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using ::testing::NiceMock;

TEST(NullSoundBackendTest, IsCalledNull)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_EQ(backend.name(), "null");
}

// It says what it is rather than pretending.
// That is what lets the conformance suite skip rather than assert a lie.
TEST(NullSoundBackendTest, ClaimsNeitherPlaybackNorAThreadOfItsOwn)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_EQ(
        backend.capabilities(),
        (SoundCapabilities{.playback = false, .selfDriven = false}));
}

TEST(NullSoundBackendTest, OpenDevice_HandsBackADeviceAtTheFormatAsked)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    const auto device = backend.openDevice(
        DeviceDesc{.format = WaveFormat{.rate = 44100, .channels = 1}});

    ASSERT_NE(device, nullptr);
    EXPECT_EQ(device->format(), (WaveFormat{.rate = 44100, .channels = 1}));
}

// A backend that plays nothing still answers as a real one does.
// Otherwise it is no use as a stand-in for one.
TEST(NullSoundBackendTest, OpenDevice_RefusesAFormatThatIsNotOne)
{
    NiceMock<MockLogger> logger;
    NullSoundBackend backend(logger);

    EXPECT_THROW(
        (void)backend.openDevice(
            DeviceDesc{.format = WaveFormat{.rate = 0, .channels = 2}}),
        SoundError);
}
