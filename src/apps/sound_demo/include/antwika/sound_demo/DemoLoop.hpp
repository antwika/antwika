#pragma once

#include <vector>

#include <antwika/sound/DeviceDesc.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/ISoundBackend.hpp>
#include <antwika/sound/PlayRequest.hpp>
#include <antwika/sound/WaveformLibrary.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::sound_demo
{

    using antwika::sound::DeviceDesc;
    using antwika::sound::FrameCount;
    using antwika::sound::ISoundBackend;
    using antwika::sound::PlayRequest;
    using antwika::sound::WaveformLibrary;
    using antwika::time::ISleeper;

    /**
     * @brief Opens a device, plays a schedule, and pumps until it ends.
     *
     * It runs for a stated number of frames rather than until something
     * closes, for the reason gfx3d_demo's frame cap exists: the default
     * build selects the null backend, which plays nothing and would
     * never report an end, and that is the build every CI leg produces.
     *
     * The whole run is pumped from this thread.  There is no audio
     * thread anywhere in this program, and the sleeping is pacing rather
     * than synchronisation -- nothing computed here waits on a device.
     */
    class DemoLoop final
    {
    public:
        /**
         * @brief Construct the loop from what it plays.
         * @param backend Opens the device.
         * @param library Owns the waveforms; **must outlive this**.
         * @param sleeper Paces the run, so the queue ahead of the
         * hardware stays short rather than holding the whole track.
         */
        DemoLoop(
            ISoundBackend &backend,
            const WaveformLibrary &library,
            ISleeper &sleeper);

        DemoLoop(const DemoLoop &) = delete;
        DemoLoop(DemoLoop &&) = delete;

        DemoLoop &operator=(const DemoLoop &) = delete;
        DemoLoop &operator=(DemoLoop &&) = delete;

        /**
         * @brief Play a schedule and pump until every frame is rendered.
         *
         * @param desc What to open the device as.
         * @param notes What to play, and when; start frames are
         * absolute, so this is handed over before a frame is rendered.
         * @param frames How many frames to render in total.
         * @throws antwika::sound::SoundError If the device cannot be
         * opened, or a note names a waveform the library does not hold
         * or one at a rate the device does not run at.
         */
        void run(
            const DeviceDesc &desc,
            const std::vector<PlayRequest> &notes,
            FrameCount frames);

        /**
         * @brief Get how many frames were rendered.
         * @return The count, which is what run() was asked for once it
         * has returned.
         */
        [[nodiscard]] FrameCount rendered() const noexcept;

    private:
        ISoundBackend &backend;
        const WaveformLibrary &library;
        ISleeper &sleeper;

        FrameCount renderedFrames = 0;
    };

} // namespace antwika::sound_demo
