#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/SoundError.hpp>
#include <antwika/sound/WaveFormat.hpp>

namespace antwika::sound::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;

    /**
     * @brief What a callback saw, so a test can assert it afterwards.
     *
     * The role gfx's ForeignTexture plays in its own suite: a helper the
     * shared tests need, kept beside them rather than duplicated per
     * backend.
     */
    class RecordingCallback final : public IRenderCallback
    {
    public:
        struct Call
        {
            FrameIndex firstFrame = 0;
            FrameCount frames = 0;
            std::size_t channels = 0;
            bool complete = false;
        };

        void render(SampleBuffer out, FrameIndex firstFrame) noexcept
            override
        {
            if (inside)
            {
                reentered = true;
            }

            inside = true;

            calls.push_back(
                Call{
                    .firstFrame = firstFrame,
                    .frames = out.frames,
                    .channels = out.channels.size(),
                    .complete = out.isComplete()});

            out.silence();
            inside = false;
        }

        std::vector<Call> calls;
        bool reentered = false;

    private:
        bool inside = false;
    };

    /**
     * @brief Every promise ISoundBackend makes, as one test suite.
     *
     * Backends under backends/ cannot be held to the coverage gate,
     * because CI has no sound card and no framework installed. This
     * suite is what replaces that: a backend is finished when it passes
     * this unmodified.
     *
     * Instantiate it with a traits type exposing
     * `static std::unique_ptr<ISoundBackend> create(ILogger &)`:
     *
     *     INSTANTIATE_TYPED_TEST_SUITE_P(Sdl3, SoundBackendConformance,
     *                                    Traits);
     *
     * Include this header only from a file that instantiates it, since
     * GoogleTest fails a suite that is registered and never instantiated.
     *
     * **What it deliberately does not assert**: that anything was
     * *heard*. There is no portable way to ask, and a suite that
     * insisted would force an honest backend to lie. So it asserts
     * invariants -- what a device reports, what a callback is handed,
     * and when it is called -- rather than reactions.
     */
    template <typename BackendTraits>
    class SoundBackendConformance : public ::testing::Test
    {
    protected:
        [[nodiscard]] DeviceDesc usable() const
        {
            return DeviceDesc{
                .format = WaveFormat{.rate = 48000, .channels = 2},
                .preferredBufferFrames = 256};
        }

        ::testing::NiceMock<MockLogger> logger;
        std::unique_ptr<ISoundBackend> backend{
            BackendTraits::create(logger)};
    };

    TYPED_TEST_SUITE_P(SoundBackendConformance);

    TYPED_TEST_P(SoundBackendConformance, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(SoundBackendConformance, Name_DoesNotChange)
    {
        EXPECT_EQ(this->backend->name(), this->backend->name());
    }

    TYPED_TEST_P(SoundBackendConformance, Capabilities_DoNotChange)
    {
        EXPECT_EQ(
            this->backend->capabilities(), this->backend->capabilities());
    }

    TYPED_TEST_P(SoundBackendConformance, OpenDevice_ReturnsADevice)
    {
        EXPECT_NE(this->backend->openDevice(this->usable()), nullptr);
    }

    TYPED_TEST_P(SoundBackendConformance, OpenDevice_ReportsAUsableFormat)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_TRUE(device->format().isValid());
    }

    TYPED_TEST_P(
        SoundBackendConformance, OpenDevice_ReportsANonZeroBufferSize)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_GT(device->bufferFrames(), 0U);
    }

    TYPED_TEST_P(SoundBackendConformance, OpenDevice_RefusesAZeroRate)
    {
        auto desc = this->usable();
        desc.format.rate = 0;

        EXPECT_THROW(
            (void)this->backend->openDevice(desc), antwika::sound::SoundError);
    }

    TYPED_TEST_P(SoundBackendConformance, OpenDevice_RefusesZeroChannels)
    {
        auto desc = this->usable();
        desc.format.channels = 0;

        EXPECT_THROW(
            (void)this->backend->openDevice(desc), antwika::sound::SoundError);
    }

    TYPED_TEST_P(SoundBackendConformance, Start_RefusesASecondStart)
    {
        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);

        EXPECT_THROW(
            device->start(callback), antwika::sound::SoundError);

        device->stop();
    }

    TYPED_TEST_P(SoundBackendConformance, Stop_IsSafeWhenNotStarted)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_NO_THROW(device->stop());
    }

    TYPED_TEST_P(SoundBackendConformance, Stop_IsSafeTwice)
    {
        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);
        device->stop();

        EXPECT_NO_THROW(device->stop());
    }

    TYPED_TEST_P(SoundBackendConformance, FramesPlayed_StartsAtZero)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_EQ(device->framesPlayed(), 0U);
    }

    TYPED_TEST_P(SoundBackendConformance, FramesPlayed_NeverGoesBackwards)
    {
        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);

        auto last = device->framesPlayed();

        for (int again = 0; again < 4; ++again)
        {
            (void)device->pump(128);

            const auto now = device->framesPlayed();
            EXPECT_GE(now, last);
            last = now;
        }

        device->stop();
    }

    TYPED_TEST_P(SoundBackendConformance, Pump_RendersExactlyWhatWasAsked)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);

        EXPECT_EQ(device->pump(1000), 1000U);
        EXPECT_EQ(device->framesPlayed(), 1000U);

        device->stop();
    }

    TYPED_TEST_P(SoundBackendConformance, Pump_RendersNothingBeforeAStart)
    {
        const auto device = this->backend->openDevice(this->usable());

        EXPECT_EQ(device->pump(128), 0U);
    }

    TYPED_TEST_P(SoundBackendConformance, Render_IsNeverReentered)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);
        (void)device->pump(1000);
        device->stop();

        EXPECT_FALSE(callback.reentered);
    }

    // The mistake every real device makes.
    // Restarting the counter per buffer misplaces every scheduled sound.
    TYPED_TEST_P(
        SoundBackendConformance, Render_ReceivesAscendingContiguousFrames)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);
        (void)device->pump(1000);
        device->stop();

        ASSERT_FALSE(callback.calls.empty());
        EXPECT_EQ(callback.calls.front().firstFrame, 0U);

        FrameIndex expected = 0;

        for (const auto &call : callback.calls)
        {
            EXPECT_EQ(call.firstFrame, expected);
            expected += call.frames;
        }
    }

    TYPED_TEST_P(SoundBackendConformance, Render_ReceivesACompleteBuffer)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);
        (void)device->pump(1000);
        device->stop();

        ASSERT_FALSE(callback.calls.empty());

        for (const auto &call : callback.calls)
        {
            EXPECT_TRUE(call.complete);
            EXPECT_EQ(call.channels, device->format().channels);
            EXPECT_GT(call.frames, 0U);
        }
    }

    TYPED_TEST_P(SoundBackendConformance, Render_IsNotCalledAfterStop)
    {
        if (this->backend->capabilities().selfDriven)
        {
            GTEST_SKIP() << "backend renders on a thread of its own";
        }

        const auto device = this->backend->openDevice(this->usable());
        RecordingCallback callback;

        device->start(callback);
        (void)device->pump(256);
        device->stop();

        const auto seen = callback.calls.size();

        (void)device->pump(256);

        EXPECT_EQ(callback.calls.size(), seen);
    }

    // The callback outlives the device.
    // Every caller relies on that order when a device goes out of scope.
    TYPED_TEST_P(SoundBackendConformance, Destructor_StopsAStartedDevice)
    {
        RecordingCallback callback;

        {
            const auto device = this->backend->openDevice(this->usable());
            device->start(callback);
            (void)device->pump(128);
        }

        SUCCEED();
    }

    REGISTER_TYPED_TEST_SUITE_P(
        SoundBackendConformance,
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
        Pump_RendersExactlyWhatWasAsked,
        Pump_RendersNothingBeforeAStart,
        Render_IsNeverReentered,
        Render_ReceivesAscendingContiguousFrames,
        Render_ReceivesACompleteBuffer,
        Render_IsNotCalledAfterStop,
        Destructor_StopsAStartedDevice);

} // namespace antwika::sound::conformance
