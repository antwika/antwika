#pragma once

#include <cstdint>

namespace antwika::raylib
{

    namespace detail
    {
        // Inline, because the gfx and input backends are two targets.
        // A definition in each would be two counters at the final link.
        // An inline variable is one, however many targets include it.
        inline std::uint64_t presented = 0;
    } // namespace detail

    /**
     * @brief How many frames the process's one raylib window has
     * presented.
     *
     * raylib's input state advances only inside EndDrawing, so this
     * counter is the input backend's only way to tell one frame's
     * wheel reading from the next frame's identical one -- two honest
     * one-notch frames and one frame read twice are otherwise the
     * same value.  Process-global exactly as the window it describes:
     * raylib keeps its one window in global state, and this counter
     * is a fact about that window.
     *
     * @return The count of frames presented so far.
     */
    [[nodiscard]] inline std::uint64_t frameCount() noexcept
    {
        return detail::presented;
    }

    /**
     * @brief Advance the counter.
     *
     * RaylibRenderer::present() is the one caller, right where
     * EndDrawing runs -- the only place raylib's input state moves.
     */
    inline void advanceFrame() noexcept
    {
        ++detail::presented;
    }

} // namespace antwika::raylib
