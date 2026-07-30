#include "Sdl3Renderer.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Glyphs.hpp>
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

    void Sdl3Renderer::drawText(
        Point origin,
        std::string_view text,
        std::uint32_t scale,
        Color color)
    {
        if (renderer == nullptr || scale == 0)
        {
            return;
        }

        const auto step = static_cast<float>(scale);
        std::vector<SDL_FRect> pixels;
        pixels.reserve(text.size() * kGlyphWidth * kGlyphHeight);

        for (std::size_t cell = 0; cell < text.size(); ++cell)
        {
            const auto left =
                static_cast<float>(origin.x)
                + static_cast<float>(cell * kGlyphAdvance * scale);

            for (std::uint32_t row = 0; row < kGlyphHeight; ++row)
            {
                const auto bits = glyphRow(text[cell], row);

                for (std::uint32_t column = 0; column < kGlyphWidth;
                     ++column)
                {
                    const auto shift = kGlyphWidth - 1 - column;
                    if (((bits >> shift) & 1U) == 0)
                    {
                        continue;
                    }

                    pixels.push_back(SDL_FRect{
                        .x = left + static_cast<float>(column * scale),
                        .y = static_cast<float>(origin.y)
                             + static_cast<float>(row * scale),
                        .w = step,
                        .h = step});
                }
            }
        }

        if (pixels.empty())
        {
            return;
        }

        if (!SDL_SetRenderDrawColor(
                renderer, color.red, color.green, color.blue, color.alpha))
        {
            warn(logger, "could not set the draw colour");
            return;
        }

        if (!SDL_RenderFillRects(
                renderer,
                pixels.data(),
                static_cast<int>(pixels.size())))
        {
            warn(logger, "could not fill a run of glyph pixels");
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
