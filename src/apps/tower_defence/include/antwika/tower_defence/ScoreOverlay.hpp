#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::tower_defence
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief The score bar's picture, handed from the tick path to the
     * renderer.
     *
     * The one thing two collaborators have to agree on: ScoreSink writes
     * it once per tick, RenderSink paints it over the battle.
     * A small shared state object rather than one asking the other, so
     * the renderer never has to know what a pointer or a widget is --
     * the same shape as game::UiOverlay and life::DragState.
     *
     * It holds nothing of its own: what goes in is described from the
     * simulation state, so a replay rebuilds the same picture.
     */
    class ScoreOverlay final
    {
    public:
        /**
         * @brief Construct the overlay over the size the UI belongs to.
         *
         * The canvas lives here rather than with whoever describes the
         * bar, so nothing can lay it out against one size and paint it
         * against another.
         * It is the size the window was *asked* for, never the size a
         * window reports.
         *
         * @param canvas The area the UI is laid out into.
         */
        explicit ScoreOverlay(Size canvas = {});

        /**
         * @brief Get the area the UI is laid out into.
         * @return The canvas this overlay was constructed over.
         */
        [[nodiscard]] Size canvas() const noexcept;

        /**
         * @brief Put this tick's UI in, replacing whatever was there.
         * @param picture The drawing commands, in the order they draw.
         */
        void set(DrawList picture);

        /**
         * @brief Get the UI's picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

    private:
        Size area;
        DrawList picture;
    };

} // namespace antwika::tower_defence
