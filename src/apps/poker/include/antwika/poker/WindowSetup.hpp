#pragma once

#include <chrono>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::poker
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::IGfxBackend;
    using antwika::gfx::Size;
    using antwika::time::ISleeper;

    /**
     * @brief What a session needs in order to be watched.
     *
     * One struct rather than three parameters, so a backend without a
     * sleeper cannot be asked for.
     */
    struct WindowSetup
    {
        /**
         * @brief Opens the window and reports its events.
         *
         * Must outlive the bootstrap call.
         */
        IGfxBackend &backend;

        /**
         * @brief Paces the frames.
         *
         * Must outlive the bootstrap call.
         */
        ISleeper &sleeper;

        /**
         * @brief How long to hold each tick's frame.
         *
         * Zero draws frames as fast as the ticks arrive.
         * Waiting changes only how long a run takes, never what it
         * computes, so any value here reaches the same chip counts.
         */
        std::chrono::milliseconds framePeriod{};

        /**
         * @brief Whether the last frame is kept up until the window goes.
         *
         * A window that vanished on the last tick would hide the end, so
         * a spectator wants it held.
         * It is its own answer rather than "framePeriod is non-zero",
         * because a paced *terminal* run is an ordinary thing to ask for
         * and holding there would hang under a backend that never
         * reports a close -- which is exactly what the headless one does.
         */
        bool holdFinalFrame{false};

        /**
         * @brief The decoded atlas the table is drawn from, if any.
         *
         * A pointer rather than a value, and null is an ordinary state:
         * antwika::gfx opens no files, so somebody has to have read the
         * PNG, and a test that only wants to know a session's chip
         * counts should not have to.
         * The texture is uploaded by whoever owns the renderer, since a
         * texture belongs to the renderer that made it.
         *
         * Must outlive the bootstrap call when set.
         */
        const Bitmap *atlas = nullptr;

        /**
         * @brief How big the window should be.
         */
        Size size{.width = 1024, .height = 640};
    };

} // namespace antwika::poker
