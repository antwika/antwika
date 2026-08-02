#include "RaylibClipboard.hpp"

#include <raylib.h>

#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::input::raylib
{

    using antwika::log::Level;

    // The logger is used here and not kept.
    // Nothing after construction has anything to say.
    RaylibClipboard::RaylibClipboard(ILogger &logger)
    {
        logger.log(Level::Debug, "input.raylib: holding the clipboard");
    }

    std::string RaylibClipboard::text() const
    {
        // The clipboard lives on the window.
        // Before one is up there is nothing to read.
        if (!IsWindowReady())
        {
            return {};
        }

        // raylib may answer with null for an empty clipboard.
        const char *held = GetClipboardText();

        return held != nullptr ? std::string(held) : std::string{};
    }

    void RaylibClipboard::setText(const std::string_view text)
    {
        // Dropped rather than thrown, like a draw before a window.
        // The copy still lives in the caller's own state.
        if (!IsWindowReady())
        {
            return;
        }

        SetClipboardText(std::string(text).c_str());
    }

} // namespace antwika::input::raylib
