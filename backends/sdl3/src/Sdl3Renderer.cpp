#include "Sdl3Renderer.hpp"

#include <string>

#include <antwika/log/Level.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::Level;

    namespace
    {
        void warn(ILogger &logger, const char *what)
        {
            logger.log(
                Level::Warning,
                std::string("gfx.sdl3: ") + what + ": " + SDL_GetError());
        }
    } // namespace

    Sdl3Renderer::Sdl3Renderer(ILogger &logger, SDL_Renderer *renderer)
        : logger(logger), renderer(renderer)
    {
    }

    void Sdl3Renderer::clear(Color color)
    {
        if (renderer == nullptr)
        {
            return;
        }

        if (!SDL_SetRenderDrawColor(
                renderer, color.red, color.green, color.blue, color.alpha))
        {
            warn(logger, "could not set the draw colour");
            return;
        }

        if (!SDL_RenderClear(renderer))
        {
            warn(logger, "could not clear");
        }
    }

    void Sdl3Renderer::drawRect(Rect rect, Color color)
    {
        if (renderer == nullptr)
        {
            return;
        }

        if (!SDL_SetRenderDrawColor(
                renderer, color.red, color.green, color.blue, color.alpha))
        {
            warn(logger, "could not set the draw colour");
            return;
        }

        const SDL_FRect target{
            .x = static_cast<float>(rect.origin.x),
            .y = static_cast<float>(rect.origin.y),
            .w = static_cast<float>(rect.size.width),
            .h = static_cast<float>(rect.size.height)};

        if (!SDL_RenderFillRect(renderer, &target))
        {
            warn(logger, "could not fill a rectangle");
        }
    }

    void Sdl3Renderer::present()
    {
        if (renderer == nullptr)
        {
            return;
        }

        if (!SDL_RenderPresent(renderer))
        {
            warn(logger, "could not present");
        }
    }

    void Sdl3Renderer::detach()
    {
        renderer = nullptr;
    }

} // namespace antwika::gfx::sdl3
