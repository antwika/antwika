#pragma once

#include <SDL3/SDL.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::gfx::sdl3
{

    using antwika::log::ILogger;

    /**
     * @brief Draws into one SDL window through an SDL renderer.
     *
     * Drawing failures are logged, never thrown. IRenderer documents no
     * exception on any drawing call, and a frame SDL declined to draw is
     * not worth tearing a running program down for.
     */
    class Sdl3Renderer final : public IRenderer
    {
    public:
        /**
         * @brief Construct the renderer.
         * @param logger Receives warnings about declined draw calls.
         * @param renderer The SDL renderer, owned by the window.
         */
        Sdl3Renderer(ILogger &logger, SDL_Renderer *renderer);

        Sdl3Renderer(const Sdl3Renderer &) = delete;
        Sdl3Renderer(Sdl3Renderer &&) = delete;

        Sdl3Renderer &operator=(const Sdl3Renderer &) = delete;
        Sdl3Renderer &operator=(Sdl3Renderer &&) = delete;

        /**
         * @brief Fill the whole drawable area with one colour.
         * @param color The colour to fill with.
         */
        void clear(Color color) override;

        /**
         * @brief Fill a rectangle with one colour.
         * @param rect The rectangle to fill.
         * @param color The colour to fill it with.
         */
        void drawRect(Rect rect, Color color) override;

        /**
         * @brief Present everything drawn since the last present.
         */
        void present() override;

        /**
         * @brief Forget the SDL renderer, which the window has destroyed.
         *
         * Every later drawing call becomes a no-op rather than a use of
         * freed memory, because a closed window's renderer stays
         * reachable through IWindow::renderer().
         */
        void detach();

    private:
        ILogger &logger;
        SDL_Renderer *renderer;
    };

} // namespace antwika::gfx::sdl3
