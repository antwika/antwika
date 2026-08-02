#pragma once

#include <string>
#include <string_view>

#include <antwika/input/IClipboard.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Runtime.hpp"

namespace antwika::input::sdl3
{

    using antwika::log::ILogger;
    using antwika::sdl3::Sdl3Subsystem;

    /**
     * @brief IClipboard backed by SDL3's clipboard.
     *
     * SDL keeps the clipboard in its video subsystem, so this claims
     * video exactly as Sdl3Pump does; whichever of the two arrives
     * first starts it and whichever leaves last shuts it down.
     *
     * A read that fails answers with nothing rather than throwing,
     * because an empty clipboard and an unreadable one call for the
     * same behaviour above: no paste.  A write that fails is logged
     * and dropped on the same reasoning -- the copy still lives in the
     * application's own state, and rendering does not throw either.
     */
    class Sdl3Clipboard final : public IClipboard
    {
    public:
        /**
         * @brief Claim SDL's video subsystem for the clipboard.
         * @param logger Receives the diagnostics; must outlive this
         * object.
         * @throws antwika::sdl3::Sdl3Error If SDL failed to start.
         */
        explicit Sdl3Clipboard(ILogger &logger);

        /**
         * @brief Get what the window system's clipboard holds.
         * @return The characters, empty when it holds none.
         */
        [[nodiscard]] std::string text() const override;

        /**
         * @brief Replace what the window system's clipboard holds.
         * @param text The characters to hold.
         */
        void setText(std::string_view text) override;

    private:
        ILogger &logger;
        Sdl3Subsystem video;
    };

} // namespace antwika::input::sdl3
