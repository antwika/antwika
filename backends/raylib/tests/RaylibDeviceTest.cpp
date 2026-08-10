#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <raylib.h>

#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/sound/DeviceDesc.hpp>
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

        constexpr FrameCount kBuffer = 1024;

        constexpr FrameCount kLead = 4800;

        constexpr std::chrono::milliseconds kTick{25};

        class FakeSilentCallback final : public IRenderCallback
        {
        public:
            void render(
                const SampleBuffer out, FrameIndex) noexcept override
            {
                out.silence();
            }
        };

        [[nodiscard]] DeviceDesc playbackDesc()
        {
            return DeviceDesc{
                .format = WaveFormat{.rate = 48000, .channels = 2},
                .preferredBufferFrames = kBuffer};
        }

        /**
         * @brief Answers whether this machine's audio device plays.
         *
         * @return True where a written buffer comes back finished.
         *
         * Requires: an audio device is already open.
         *
         * A container with no sound hardware opens a device that
         * accepts buffers and never finishes one, and no caller can
         * tell a stall from silence on such a device.
         */
        [[nodiscard]] bool platformFinishesBuffers()
        {
            SetAudioStreamBufferSizeDefault(static_cast<int>(kBuffer));

            const AudioStream probe = LoadAudioStream(48000, 32, 2);

            if (!IsAudioStreamValid(probe))
            {
                return false;
            }

            PlayAudioStream(probe);

            const std::vector<float> silence(kBuffer * 2, 0.0F);

            for (std::size_t written = 0; written < 2; ++written)
            {
                if (IsAudioStreamProcessed(probe))
                {
                    UpdateAudioStream(
                        probe, silence.data(), static_cast<int>(kBuffer));
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

    TEST(RaylibDeviceTest, FramesPlayed_AdvancesWhileALeadedPumpFeedsIt)
    {
        NiceMock<MockLogger> logger;
        RaylibSoundBackend backend(logger);

        if (!backend.capabilities().playback)
        {
            GTEST_SKIP() << "no playback device on this machine";
        }

        if (!platformFinishesBuffers())
        {
            GTEST_SKIP()
                << "this machine's audio device finishes no buffers, so "
                   "it cannot show playback stalling";
        }

        const auto device = backend.openDevice(playbackDesc());
        FakeSilentCallback callback;

        device->start(callback);

        FrameCount queued = 0;

        for (std::size_t tick = 0; tick < 40; ++tick)
        {
            const auto played = device->framesPlayed();
            const auto ahead = queued > played ? queued - played : 0;

            if (ahead < kLead)
            {
                queued += device->pump(kLead - ahead);
            }

            std::this_thread::sleep_for(kTick);
        }

        EXPECT_GT(device->framesPlayed(), kBuffer * 2);

        device->stop();
    }

}
