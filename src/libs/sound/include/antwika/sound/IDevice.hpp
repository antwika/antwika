#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound
{

    /**
     * @brief One open audio device.
     *
     * Started with a callback, stopped, and -- unless it drives itself --
     * pumped for however many frames a caller wants rendered.
     */
    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        /**
         * @brief Begin rendering through a callback.
         * @param callback Asked for every frame from now until stop();
         * **must outlive that interval**.
         * @throws SoundError If the device is already started, or the
         * underlying framework refused.
         */
        virtual void start(IRenderCallback &callback) = 0;

        /**
         * @brief Stop rendering.
         *
         * Idempotent, and safe on a device that was never started, so a
         * caller unwinding does not have to know which it has.
         * Once this returns, the callback is no longer in use.
         */
        virtual void stop() = 0;

        /**
         * @brief Render and consume some frames.
         *
         * The whole reason this library needs no thread of its own: a
         * pumped device does nothing until it is asked, so a headless
         * run is instantaneous and a test spends no wall-clock time.
         *
         * @param frames How many to render.
         * @return How many were rendered, which is zero for a device
         * that drives itself and zero for one that is not started.
         */
        virtual FrameCount pump(FrameCount frames) = 0;

        /**
         * @brief Get the format this device was opened at.
         * @return The format, which is what a callback must write.
         */
        [[nodiscard]] virtual WaveFormat format() const = 0;

        /**
         * @brief Get how many frames it asks for at a time.
         * @return The buffer size it actually got, never zero.
         */
        [[nodiscard]] virtual FrameCount bufferFrames() const = 0;

        /**
         * @brief Get how many frames the device has consumed.
         *
         * **Monotonic and advisory.** It is legal to read this only to
         * decide how long to sleep, and never to decide what to compute.
         *
         * That is the single exception to information flowing from the
         * simulation to the audio and never back, and it is written here
         * rather than in a design document because here is where
         * somebody about to break it is looking.
         *
         * It is what has been *consumed*, not what has been handed over,
         * and the difference is the whole point: a device that consumes
         * instantly reports every frame pumped, while a real one lags by
         * whatever is still queued ahead of the hardware.  Pacing needs
         * that lag -- a caller told it had played everything it pushed
         * would never have a reason to wait.
         *
         * @return The count, which never decreases and never exceeds
         * what has been pumped.
         */
        [[nodiscard]] virtual FrameIndex framesPlayed() const = 0;
    };

} // namespace antwika::sound
