#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <raylib.h>

#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/sound/DeviceSpec.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IRenderCallback.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/WaveFormat.hpp>

#include "RaylibSoundBackend.hpp"

namespace antwika::sound
{

    namespace
    {
        using antwika::log::mocks::MockLogger;
        using ::testing::NiceMock;

        constexpr FrameCount kBufferCount = 1024;

        constexpr FrameCount kLeadCount = 4800;

        constexpr std::chrono::milliseconds kTick{25};

        class FakeSilentCallback final : public IRenderCallback
        {
        public:
            void render(
                const SampleBuffer samples, FrameIndex) noexcept override
            {
                samples.silence();
            }
        };

        [[nodiscard]] DeviceSpec getPlaybackSpec()
        {
            return DeviceSpec{
                .format = WaveFormat{.rate = 48000, .channels = 2},
                .preferredBufferFrames = kBufferCount};
        }

        [[nodiscard]] bool isPlatformFinishesBuffers()
        {
            SetAudioStreamBufferSizeDefault(static_cast<int>(kBufferCount));

            const AudioStream probe = LoadAudioStream(48000, 32, 2);

            if (!IsAudioStreamValid(probe))
            {
                return false;
            }

            PlayAudioStream(probe);

            const std::vector<float> silence(kBufferCount * 2, 0.0F);

            for (std::size_t roundIndex = 0; roundIndex < 2; ++roundIndex)
            {
                if (IsAudioStreamProcessed(probe))
                {
                    UpdateAudioStream(
                        probe, silence.data(), static_cast<int>(kBufferCount));
                }
            }

            bool finished = false;

            for (std::size_t wait = 0; wait < 20 && !finished; ++wait)
            {
                std::this_thread::sleep_for(kTick);
                finished = IsAudioStreamProcessed(probe);
            }

            StopAudioStream(probe);
            UnloadAudioStream(probe);

            return finished;
        }
    }

    TEST(RaylibDeviceTest, FramesPlayed_AdvancesWhileALeadedAdvanceFeedsIt)
    {
        NiceMock<MockLogger> logger;
        RaylibSoundBackend backend(logger);

        if (!backend.getCapabilities().playback)
        {
            GTEST_SKIP() << "no playback device on this machine";
        }

        if (!isPlatformFinishesBuffers())
        {
            GTEST_SKIP()
                << "this machine's audio device finishes no buffers, so "
                   "it cannot show playback stalling";
        }

        const auto device = backend.openDevice(getPlaybackSpec());
        FakeSilentCallback callback;

        device->start(callback);

        FrameCount queuedCount = 0;

        for (std::size_t tick = 0; tick < 40; ++tick)
        {
            const auto playedFrames = device->getFramesPlayed();
            const auto ahead =
                queuedCount > playedFrames ? queuedCount - playedFrames : 0;

            if (ahead < kLeadCount)
            {
                queuedCount += device->advance(kLeadCount - ahead);
            }

            std::this_thread::sleep_for(kTick);
        }

        EXPECT_GT(device->getFramesPlayed(), kBufferCount * 2);

        device->stop();
    }

}
