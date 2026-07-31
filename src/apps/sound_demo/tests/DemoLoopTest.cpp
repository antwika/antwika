#include "antwika/sound_demo/DemoLoop.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/NullSoundBackend.hpp>
#include <antwika/sound/SoundCapabilities.hpp>
#include <antwika/sound/SoundError.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/sound/WaveformLibrary.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/sound_demo/DemoTrack.hpp"

using antwika::log::mocks::MockLogger;
using antwika::sound::DeviceDesc;
using antwika::sound::NullSoundBackend;
using antwika::sound::PlayRequest;
using antwika::sound::SoundError;
using antwika::sound::WaveFormat;
using antwika::sound::WaveformLibrary;
using antwika::sound_demo::demoSchedule;
using antwika::sound_demo::DemoLoop;
using antwika::sound_demo::demoTone;
using antwika::time::fakes::FakeSleeper;

namespace
{
    constexpr WaveFormat kStereo{.rate = 48000, .channels = 2};

    /**
     * @brief A device that consumes behind what it is handed.
     *
     * No backend in this tree lags: the null one consumes instantly and
     * the offline one writes straight into a waveform, so every pacing
     * path in DemoLoop is unreachable through either of them.
     *
     * That is the whole reason this exists rather than a real backend
     * being used here.  A real device is exactly the case the pacing was
     * written for, and it is the one case a test could not otherwise
     * reach without a sound card and a wall clock.
     */
    class LaggingDevice final : public antwika::sound::IDevice
    {
    public:
        LaggingDevice(
            antwika::sound::FrameCount lag, bool catchesUp) noexcept
            : step(lag / 100 + 1), catchesUp(catchesUp), outstanding(lag)
        {
        }

        void start(antwika::sound::IRenderCallback &callback) override
        {
            sink = &callback;
        }

        void stop() override
        {
            sink = nullptr;
        }

        antwika::sound::FrameCount pump(
            antwika::sound::FrameCount frames) override
        {
            if (sink == nullptr)
            {
                return 0;
            }

            pushed += frames;
            return frames;
        }

        [[nodiscard]] WaveFormat format() const override
        {
            return kStereo;
        }

        [[nodiscard]] antwika::sound::FrameCount bufferFrames()
            const override
        {
            return 256;
        }

        // Behind by however much is still outstanding.
        // The gap closes a little per reading if built to close.
        //
        // Closing gradually makes the loop lag and then stop lagging.
        // That is the shape a real device has.
        // One that never closes proves the drain wait is bounded.
        [[nodiscard]] antwika::sound::FrameIndex framesPlayed()
            const override
        {
            const auto behind = std::min(outstanding, pushed);
            const auto played = pushed - behind;

            if (catchesUp)
            {
                outstanding -= std::min(outstanding, step);
            }

            return played;
        }

    private:
        antwika::sound::FrameCount step;
        bool catchesUp;

        antwika::sound::IRenderCallback *sink = nullptr;
        antwika::sound::FrameIndex pushed = 0;

        mutable antwika::sound::FrameIndex outstanding = 0;
    };

    /**
     * @brief Hands out one LaggingDevice.
     */
    class LaggingBackend final : public antwika::sound::ISoundBackend
    {
    public:
        LaggingBackend(
            antwika::sound::FrameCount lag, bool catchesUp) noexcept
            : lag(lag), catchesUp(catchesUp)
        {
        }

        [[nodiscard]] std::string_view name() const override
        {
            return "lagging";
        }

        [[nodiscard]] antwika::sound::SoundCapabilities capabilities()
            const override
        {
            return antwika::sound::SoundCapabilities{
                .playback = true, .selfDriven = false};
        }

        [[nodiscard]] std::unique_ptr<antwika::sound::IDevice> openDevice(
            const DeviceDesc &desc) override
        {
            if (!desc.format.isValid())
            {
                throw SoundError("lagging: an unusable format");
            }

            return std::make_unique<LaggingDevice>(lag, catchesUp);
        }

    private:
        antwika::sound::FrameCount lag;
        bool catchesUp;
    };

    /**
     * @brief A loop over the null backend, with no time really spent.
     *
     * The whole app is exercised here without a sound card, which is the
     * point of the backend seam being what it is.
     */
    class DemoLoopTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        NullSoundBackend backend{logger};
        WaveformLibrary library;
        FakeSleeper sleeper;
    };
} // namespace

TEST_F(DemoLoopTest, RendersExactlyTheFramesItWasAskedFor)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 5000);

    EXPECT_EQ(loop.rendered(), 5000U);
}

TEST_F(DemoLoopTest, RendersNothingWhenAskedForNothing)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 0);

    EXPECT_EQ(loop.rendered(), 0U);
}

// A track with no notes is silence rather than a failure.
TEST_F(DemoLoopTest, PlaysAnEmptyScheduleWithoutComplaining)
{
    DemoLoop loop(backend, library, sleeper);

    loop.run(DeviceDesc{.format = kStereo}, {}, 1024);

    EXPECT_EQ(loop.rendered(), 1024U);
}

// A device that consumes instantly is never ahead of itself.
// So nothing here ever has a reason to wait.
TEST_F(DemoLoopTest, DoesNotPaceADeviceThatKeepsUp)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 20000);

    EXPECT_TRUE(sleeper.requested().empty());
}

TEST_F(DemoLoopTest, RefusesADeviceItCouldNotBeOpenedAs)
{
    DemoLoop loop(backend, library, sleeper);

    EXPECT_THROW(
        loop.run(
            DeviceDesc{.format = WaveFormat{.rate = 0, .channels = 2}},
            {},
            1024),
        SoundError);
}

// A note at another rate is refused, not played at the wrong speed.
// The refusal arrives before a frame is rendered.
// That is because the whole schedule is handed over first.
TEST_F(DemoLoopTest, RefusesANoteAtTheWrongRate)
{
    const auto slow = library.add(
        demoTone(WaveFormat{.rate = 22050, .channels = 2}, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    EXPECT_THROW(
        loop.run(
            DeviceDesc{.format = kStereo}, demoSchedule(slow, 480), 1024),
        SoundError);

    EXPECT_EQ(loop.rendered(), 0U);
}

// The pacing branch, which no backend in this tree can reach.
// A device that consumes instantly is never far enough ahead.
TEST_F(DemoLoopTest, WaitsWhenTheQueueRunsAheadOfTheDevice)
{
    LaggingBackend lagging{20000, true};
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(lagging, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 40000);

    EXPECT_EQ(loop.rendered(), 40000U);
    EXPECT_FALSE(sleeper.requested().empty());
}

// The drain wait is bounded rather than hopeful.
// A device that stopped consuming ends the run instead of hanging it.
TEST_F(DemoLoopTest, GivesUpWaitingForADeviceThatStopped)
{
    LaggingBackend stuck{100000, false};
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(stuck, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 2000);

    EXPECT_EQ(loop.rendered(), 2000U);
    EXPECT_FALSE(sleeper.requested().empty());
}

TEST_F(DemoLoopTest, ALaggingBackendStillRefusesAnUnusableFormat)
{
    LaggingBackend lagging{1000, true};

    DemoLoop loop(lagging, library, sleeper);

    EXPECT_THROW(
        loop.run(
            DeviceDesc{.format = WaveFormat{.rate = 48000, .channels = 0}},
            {},
            256),
        SoundError);
}
