#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string_view>
#include <vector>

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

#include "antwika/sound_demo/DemoLoop.hpp"
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

    class FakeLaggingDevice final : public antwika::sound::IDevice
    {
    public:
        FakeLaggingDevice(
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

    class FakeLaggingBackend final : public antwika::sound::ISoundBackend
    {
    public:
        FakeLaggingBackend(
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

            return std::make_unique<FakeLaggingDevice>(lag, catchesUp);
        }

    private:
        antwika::sound::FrameCount lag;
        bool catchesUp;
    };

    class DemoLoopTest : public ::testing::Test
    {
    protected:
        ::testing::NiceMock<MockLogger> logger;
        NullSoundBackend backend{logger};
        WaveformLibrary library;
        FakeSleeper sleeper;
    };
}

TEST_F(DemoLoopTest, Run_RendersExactlyTheFramesAskedFor)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 5000);

    EXPECT_EQ(loop.rendered(), 5000U);
}

TEST_F(DemoLoopTest, Run_RendersNothingWhenAskedForNothing)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 0);

    EXPECT_EQ(loop.rendered(), 0U);
}

TEST_F(DemoLoopTest, Run_PlaysAnEmptyScheduleWithoutComplaining)
{
    DemoLoop loop(backend, library, sleeper);

    loop.run(DeviceDesc{.format = kStereo}, {}, 1024);

    EXPECT_EQ(loop.rendered(), 1024U);
}

TEST_F(DemoLoopTest, Run_DoesNotPaceADeviceThatKeepsUp)
{
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(backend, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 20000);

    EXPECT_TRUE(sleeper.requested().empty());
}

TEST_F(DemoLoopTest, Run_RefusesADeviceItCouldNotOpenAs)
{
    DemoLoop loop(backend, library, sleeper);

    EXPECT_THROW(
        loop.run(
            DeviceDesc{.format = WaveFormat{.rate = 0, .channels = 2}},
            {},
            1024),
        SoundError);
}

TEST_F(DemoLoopTest, Run_RefusesANoteAtTheWrongRate)
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

TEST_F(DemoLoopTest, Run_WaitsWhenTheQueueRunsAhead)
{
    FakeLaggingBackend lagging{20000, true};
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(lagging, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 40000);

    EXPECT_EQ(loop.rendered(), 40000U);
    EXPECT_FALSE(sleeper.requested().empty());
}

TEST_F(DemoLoopTest, Run_GivesUpOnADeviceThatStopped)
{
    FakeLaggingBackend stuck{100000, false};
    const auto tone = library.add(demoTone(kStereo, 440.0, 480));

    DemoLoop loop(stuck, library, sleeper);

    loop.run(
        DeviceDesc{.format = kStereo}, demoSchedule(tone, 480), 2000);

    EXPECT_EQ(loop.rendered(), 2000U);
    EXPECT_FALSE(sleeper.requested().empty());
}

TEST_F(DemoLoopTest, Run_RefusesAnUnusableFormatWhenLagging)
{
    FakeLaggingBackend lagging{1000, true};

    DemoLoop loop(lagging, library, sleeper);

    EXPECT_THROW(
        loop.run(
            DeviceDesc{.format = WaveFormat{.rate = 48000, .channels = 0}},
            {},
            256),
        SoundError);
}
