#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Pump.hpp"

namespace antwika::input::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief IInputBackend backed by SDL3.
     *
     * Reads the keyboard and mouse events SDL already reports, off the
     * shared Sdl3Pump rather than from SDL_PollEvent directly -- see
     * Sdl3Pump for why that matters when a window is open at the same
     * time.
     *
     * SDL delivers real events, so nothing here has to diff state or
     * synthesise an edge: each SDL event becomes at most one InputEvent,
     * and the ones this vocabulary has no word for (a key no Key names, a
     * button no MouseButton names) are dropped as polling continues.
     *
     * SDL reports which window an event arrived at, and this throws that
     * away, because IInputBackend deliberately says nothing about windows.
     * For a single-window application -- which is every application here
     * -- that costs nothing.
     */
    class Sdl3InputBackend final : public IInputBackend
    {
    public:
        /**
         * @brief Take a share of the process's SDL event pump.
         * @param logger Receives the backend's diagnostics.
         * @throws InputError If SDL's video subsystem failed to start.
         */
        explicit Sdl3InputBackend(ILogger &logger);

        Sdl3InputBackend(const Sdl3InputBackend &) = delete;
        Sdl3InputBackend(Sdl3InputBackend &&) = delete;

        Sdl3InputBackend &operator=(const Sdl3InputBackend &) = delete;
        Sdl3InputBackend &operator=(Sdl3InputBackend &&) = delete;

        /**
         * @brief Get the backend's name.
         * @return Always "sdl3".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Get which devices this backend deals in.
         * @return Both of them: SDL reports a keyboard and a pointer.
         */
        [[nodiscard]] InputCapabilities capabilities() const override;

        /**
         * @brief Take the next edge SDL has reported.
         * @return The next translatable edge, or nullopt when none is
         * left.
         */
        [[nodiscard]] std::optional<InputEvent> pollEvent() override;

    private:
        std::shared_ptr<antwika::sdl3::Sdl3Pump> pump;

        // The fractions of a wheel notch not yet reported.
        // SDL reports wheels in floats; a touchpad's two-finger
        // scroll arrives in fractions, and truncating every event
        // would leave such scrolling at zero forever.
        float remainderX = 0.0F;
        float remainderY = 0.0F;
    };

} // namespace antwika::input::sdl3
