#pragma once

#include <chrono>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::poker
{

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
         * Zero means nobody asked to watch: frames are drawn as fast as
         * the ticks arrive and the last one is not held open afterwards.
         * Holding it would hang under a backend that never reports a
         * close, which is exactly what the headless one does.
         */
        std::chrono::milliseconds framePeriod{};

        /**
         * @brief How big the window should be.
         */
        Size size{.width = 1024, .height = 640};
    };

} // namespace antwika::poker
