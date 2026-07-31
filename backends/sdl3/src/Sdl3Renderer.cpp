#include "Sdl3Renderer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Blit.hpp>
#include <antwika/gfx/GfxError.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/TextRaster.hpp>
#include <antwika/log/Level.hpp>

#include "Sdl3Texture.hpp"

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

        [[noreturn]] void fail(const char *what)
        {
            throw GfxError(
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

    void Sdl3Renderer::drawLine(Point from, Point to, Color color)
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

        if (!SDL_RenderLine(
                renderer,
                static_cast<float>(from.x),
                static_cast<float>(from.y),
                static_cast<float>(to.x),
                static_cast<float>(to.y)))
        {
            warn(logger, "could not draw a line");
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

        std::vector<SDL_FRect> pixels;
        pixels.reserve(text.size() * kGlyphWidth * kGlyphHeight);

        // Where the lit pixels are is gfx's answer, not this backend's.
        // Every backend has to draw the same glyphs in the same places.
        forEachGlyphPixel(
            origin,
            text,
            scale,
            [&pixels](Rect pixel) {
                pixels.push_back(SDL_FRect{
                    .x = static_cast<float>(pixel.origin.x),
                    .y = static_cast<float>(pixel.origin.y),
                    .w = static_cast<float>(pixel.size.width),
                    .h = static_cast<float>(pixel.size.height)});
            });

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

    std::unique_ptr<ITexture> Sdl3Renderer::createTexture(
        const Bitmap &bitmap)
    {
        if (!bitmap.isComplete())
        {
            throw GfxError(
                "gfx.sdl3: bitmap does not hold the pixels it claims");
        }

        if (renderer == nullptr)
        {
            throw GfxError(
                "gfx.sdl3: the window this renderer drew into has "
                "closed");
        }

        // RGBA32 is the byte-order-correct alias.
        // It names a gfx::Bitmap's layout on either endianness.
        SDL_Texture *texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC,
            static_cast<int>(bitmap.size.width),
            static_cast<int>(bitmap.size.height));

        if (texture == nullptr)
        {
            fail("could not create a texture");
        }

        const auto pitch = static_cast<int>(
            bitmap.size.width * kBytesPerPixel);

        if (!SDL_UpdateTexture(texture, nullptr, bitmap.pixels.data(), pitch))
        {
            SDL_DestroyTexture(texture);
            fail("could not upload a texture");
        }

        // Nothing else here sets a blend mode.
        // A filled rectangle carries its alpha in the draw colour.
        // A texture does not, so it has to be asked for.
        if (!SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND))
        {
            warn(logger, "could not set a texture's blend mode");
        }

        return std::make_unique<Sdl3Texture>(
            *this, texture, bitmap.size);
    }

    void Sdl3Renderer::drawTexture(
        const ITexture &texture, Rect source, Rect destination, Color tint)
    {
        if (renderer == nullptr)
        {
            return;
        }

        // ITexture exposes no native handle, on purpose.
        // Reaching SDL's means asking whether this is even ours.
        const auto *mine = dynamic_cast<const Sdl3Texture *>(&texture);

        if (mine == nullptr || !mine->belongsTo(*this))
        {
            logger.log(
                Level::Warning,
                "gfx.sdl3: declined a texture from another renderer");
            return;
        }

        if (mine->raw() == nullptr)
        {
            return;
        }

        if (!blitIsDrawable(mine->size(), source, destination))
        {
            return;
        }

        if (!SDL_SetTextureColorMod(
                mine->raw(), tint.red, tint.green, tint.blue))
        {
            warn(logger, "could not set a texture's colour");
            return;
        }

        if (!SDL_SetTextureAlphaMod(mine->raw(), tint.alpha))
        {
            warn(logger, "could not set a texture's alpha");
            return;
        }

        const SDL_FRect from{
            .x = static_cast<float>(source.origin.x),
            .y = static_cast<float>(source.origin.y),
            .w = static_cast<float>(source.size.width),
            .h = static_cast<float>(source.size.height)};

        const SDL_FRect to{
            .x = static_cast<float>(destination.origin.x),
            .y = static_cast<float>(destination.origin.y),
            .w = static_cast<float>(destination.size.width),
            .h = static_cast<float>(destination.size.height)};

        if (!SDL_RenderTexture(renderer, mine->raw(), &from, &to))
        {
            warn(logger, "could not draw a texture");
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
        // Before the window destroys the SDL renderer they need.
        // Freeing a texture after its renderer is undefined.
        // Each is left valid but empty, since one may outlive us.
        for (Sdl3Texture *texture : liveTextures)
        {
            SDL_DestroyTexture(texture->raw());
            texture->forgetRenderer();
        }

        liveTextures.clear();

        renderer = nullptr;
    }

    void Sdl3Renderer::rememberTexture(Sdl3Texture &texture)
    {
        liveTextures.push_back(&texture);
    }

    void Sdl3Renderer::forgetTexture(const Sdl3Texture &texture) noexcept
    {
        std::erase(liveTextures, &texture);
    }

} // namespace antwika::gfx::sdl3
