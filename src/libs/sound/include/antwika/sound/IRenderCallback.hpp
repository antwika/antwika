#pragma once

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/SampleBuffer.hpp"

namespace antwika::sound
{

    /**
     * @brief Fills a device's next buffer with audio.
     *
     * The one thing a device asks of an application.
     *
     * **noexcept is the contract rather than a hint.** A real device may
     * call this from a thread that cannot unwind, and even a pumped one
     * is a path where throwing would leave a buffer half written.
     *
     * The frame index is **absolute**, counted from when the device
     * started, so a sound scheduled for an exact moment lands on it
     * whatever buffer boundary happens to fall nearby.
     * The commonest mistake a device makes is restarting that counter
     * per buffer, which puts every scheduled sound on the wrong frame,
     * and the conformance suite exists partly to catch it.
     */
    class IRenderCallback
    {
    public:
        virtual ~IRenderCallback() = default;

        /**
         * @brief Write the next frames.
         * @param out Where to write; every channel holds out.frames.
         * @param firstFrame The absolute index of out's first frame.
         */
        virtual void render(
            SampleBuffer out, FrameIndex firstFrame) noexcept = 0;
    };

} // namespace antwika::sound
