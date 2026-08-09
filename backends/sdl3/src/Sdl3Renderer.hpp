#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/GlyphSheetTextures.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    class Sdl3Texture;

    class Sdl3Renderer final : public IRenderer
    {
    public:
        Sdl3Renderer(ILogger &logger, SDL_Renderer *renderer);

        Sdl3Renderer(const Sdl3Renderer &) = delete;
        Sdl3Renderer(Sdl3Renderer &&) = delete;

        Sdl3Renderer &operator=(const Sdl3Renderer &) = delete;
        Sdl3Renderer &operator=(Sdl3Renderer &&) = delete;

        void clear(Color color) override;

        void drawRect(Rect rect, Color color) override;

        void drawLine(Point from, Point to, Color color) override;

        void drawText(
            Point origin,
            std::string_view text,
            std::uint32_t scale,
            Color color) override;

        [[nodiscard]] std::unique_ptr<ITexture> createTexture(
            const Bitmap &bitmap) override;

        void drawTexture(
            const ITexture &texture,
            Rect source,
            Rect destination,
            Color tint) override;

        void present() override;

        void detach();

        void rememberTexture(Sdl3Texture &texture);

        void forgetTexture(const Sdl3Texture &texture) noexcept;

    private:
        [[nodiscard]] bool setDrawColor(Color color, SDL_BlendMode blend);

        ILogger &logger;
        SDL_Renderer *renderer;

        GlyphSheetTextures glyphSheets;

        std::vector<Sdl3Texture *> liveTextures;
    };

}
