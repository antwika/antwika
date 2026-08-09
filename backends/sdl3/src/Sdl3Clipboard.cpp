#include "Sdl3Clipboard.hpp"

#include <SDL3/SDL.h>

#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::input::sdl3
{

    using antwika::log::Level;

    Sdl3Clipboard::Sdl3Clipboard(ILogger &logger)
        : logger(logger), video(logger, SDL_INIT_VIDEO, "video")
    {
    }

    std::string Sdl3Clipboard::text() const
    {
        char *held = SDL_GetClipboardText();

        std::string result(held);

        SDL_free(held);

        return result;
    }

    void Sdl3Clipboard::setText(const std::string_view text)
    {
        if (!SDL_SetClipboardText(std::string(text).c_str()))
        {
            logger.log(
                Level::Warning,
                "SDL could not take the clipboard: "
                    + std::string(SDL_GetError()));
        }
    }

}
