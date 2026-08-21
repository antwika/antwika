#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/sound/DeviceSpec.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/SoundError.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/fakes/FakeRecordingCallback.hpp>

namespace antwika::sound::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;

    template <typename BackendTraits>
    class SoundBackendConformanceTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] DeviceSpec usable() const
        {
            return DeviceSpec{
                .format = WaveFormat{.rate = 48000, .channels = 2},
                .preferredBufferFrames = 256};
        }

        ::testing::NiceMock<MockLogger> logger;
        std::unique_ptr<ISoundBackend> backend{
            BackendTraits::create(logger)};
    };

    TYPED_TEST_SUITE_P(SoundBackendConformanceTest);

    TYPED_TEST_P(SoundBackendConformanceTest, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Name_DoesNotChange)
    {
        EXPECT_EQ(this->backend->name(), this->backend->name());
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Capabilities_DoNotChange)
    {
        EXPECT_EQ(
            this->backend->capabilities(), this->backend->capabilities());
    }

    TYPED_TEST_P(SoundBackendConformanceTest, OpenDevice_ReturnsADevice)
    {
        EXPECT_NE(this->backend->openDevice(this->usable()), nullptr);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, OpenDevice_ReportsAUsableFormat)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_TRUE(device->format().isValid());
    }

    TYPED_TEST_P(
        SoundBackendConformanceTest, OpenDevice_ReportsANonZeroBufferSize)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_GT(device->bufferFrames(), 0U);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, OpenDevice_RefusesAZeroRate)
    {
        auto spec = this->usable();
        spec.format.rate = 0;

        EXPECT_THROW(
            (void)this->backend->openDevice(spec), antwika::sound::SoundError);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, OpenDevice_RefusesZeroChannels)
    {
        auto spec = this->usable();
        spec.format.channels = 0;

        EXPECT_THROW(
            (void)this->backend->openDevice(spec), antwika::sound::SoundError);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Start_RefusesASecondStart)
    {
        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);

        EXPECT_THROW(
            device->start(callback), antwika::sound::SoundError);

        device->stop();
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Stop_IsSafeWhenNotStarted)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_NO_THROW(device->stop());
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Stop_IsSafeTwice)
    {
        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);
        device->stop();

        EXPECT_NO_THROW(device->stop());
    }

    TYPED_TEST_P(SoundBackendConformanceTest, FramesPlayed_StartsAtZero)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_EQ(device->framesPlayed(), 0U);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, FramesPlayed_NeverGoesBackwards)
    {
        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);

        auto lastFrames = device->framesPlayed();

        for (int roundIndex = 0; roundIndex < 4; ++roundIndex)
        {
            (void)device->advance(128);

            const auto playedFrames = device->framesPlayed();
            EXPECT_GE(playedFrames, lastFrames);
            lastFrames = playedFrames;
        }

        device->stop();
    }

    TYPED_TEST_P(
    SoundBackendConformanceTest,
    Advance_RendersExactlyWhatWasAsked)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);

        EXPECT_EQ(device->advance(1000), 1000U);

        device->stop();
    }

    TYPED_TEST_P(SoundBackendConformanceTest, FramesPlayed_NeverRunsAhead)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);

        EXPECT_EQ(device->advance(1000), 1000U);
        EXPECT_LE(device->framesPlayed(), 1000U);

        device->stop();
    }

    TYPED_TEST_P(
    SoundBackendConformanceTest,
    Advance_RendersNothingBeforeAStart)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_EQ(device->advance(128), 0U);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Render_IsNeverReentered)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);
        (void)device->advance(1000);
        device->stop();

        EXPECT_FALSE(callback.reentered);
    }

    TYPED_TEST_P(
        SoundBackendConformanceTest, Render_ReceivesAscendingContiguousFrames)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);
        (void)device->advance(1000);
        device->stop();

        ASSERT_FALSE(callback.calls.empty());
        EXPECT_EQ(callback.calls.front().firstFrame, 0U);

        FrameIndex expectedIndex = 0;

        for (const auto &call : callback.calls)
        {
            EXPECT_EQ(call.firstFrame, expectedIndex);
            expectedIndex += call.frames;
        }
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Render_ReceivesACompleteBuffer)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);
        (void)device->advance(1000);
        device->stop();

        ASSERT_FALSE(callback.calls.empty());

        for (const auto &call : callback.calls)
        {
            EXPECT_TRUE(call.complete);
            EXPECT_EQ(call.channels, device->format().channels);
            EXPECT_GT(call.frames, 0U);
        }
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Render_IsNotCalledAfterStop)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        fakes::FakeRecordingCallback callback;

        device->start(callback);
        (void)device->advance(256);
        device->stop();

        const auto callCount = callback.calls.size();
        ASSERT_GT(callCount, 0U);

        (void)device->advance(256);

        EXPECT_EQ(callback.calls.size(), callCount);
    }

    TYPED_TEST_P(SoundBackendConformanceTest, Destructor_StopsAStartedDevice)
    {
        fakes::FakeRecordingCallback callback;

        {
            const auto device = this->backend->openDevice(this->usable());
            device->start(callback);
            (void)device->advance(128);
        }

        SUCCEED();
    }

    REGISTER_TYPED_TEST_SUITE_P(
        SoundBackendConformanceTest,
        Name_IsNotEmpty,
        Name_DoesNotChange,
        Capabilities_DoNotChange,
        OpenDevice_ReturnsADevice,
        OpenDevice_ReportsAUsableFormat,
        OpenDevice_ReportsANonZeroBufferSize,
        OpenDevice_RefusesAZeroRate,
        OpenDevice_RefusesZeroChannels,
        Start_RefusesASecondStart,
        Stop_IsSafeWhenNotStarted,
        Stop_IsSafeTwice,
        FramesPlayed_StartsAtZero,
        FramesPlayed_NeverGoesBackwards,
        FramesPlayed_NeverRunsAhead,
        Advance_RendersExactlyWhatWasAsked,
        Advance_RendersNothingBeforeAStart,
        Render_IsNeverReentered,
        Render_ReceivesAscendingContiguousFrames,
        Render_ReceivesACompleteBuffer,
        Render_IsNotCalledAfterStop,
        Destructor_StopsAStartedDevice);

}
