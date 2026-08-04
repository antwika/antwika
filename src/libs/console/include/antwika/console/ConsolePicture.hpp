#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::console
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The console's picture, and the canvas it is laid against.
     *
     * The one thing the sink and an application's renderer share:
     * ConsoleSink writes it once per tick, and the renderer paints it
     * last, over whichever screen is up.
     * The canvas lives here so nothing can describe the sheet against
     * one size and paint it over another -- it is the size the window
     * was *asked* for, never the size one reports.
     *
     * It holds nothing the tick path reads back: what the console
     * covers is answered by ConsoleState, off recorded input alone.
     */
    class ConsolePicture final
    {
    public:
        /**
         * @brief Construct the picture over the size the sheet drops
         * down over.
         * @param canvas The area the console is laid out into.
         */
        explicit ConsolePicture(Size canvas = {});

        /**
         * @brief Get the area the console is laid out into.
         * @return The canvas this picture was constructed over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Put this tick's picture in, replacing what was there.
         * @param picture The drawing commands, in the order they draw.
         */
        void set(DrawList picture);

        /**
         * @brief Get the console's picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

} // namespace antwika::console
