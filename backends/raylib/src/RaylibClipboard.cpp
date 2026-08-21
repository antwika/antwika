#include "RaylibClipboard.hpp"

#include <raylib.h>

#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::input::raylib
{

    using antwika::log::Level;

    RaylibClipboard::RaylibClipboard(ILogger &logger)
    {
        logger.log(Level::Debug, "input.raylib: holding the clipboard");
    }

    std::string RaylibClipboard::text() const
    {
        if (!IsWindowReady())
        {
            return {};
        }

        const char *contents = GetClipboardText();

        return contents != nullptr ? std::string(contents)
                                   : std::string{};
    }

    void RaylibClipboard::setText(const std::string_view text)
    {
        if (!IsWindowReady())
        {
            return;
        }

        SetClipboardText(std::string(text).c_str());
    }

}
