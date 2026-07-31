#pragma once

#include <antwika/ui/DrawList.hpp>

namespace antwika::atlas_editor
{

    using antwika::ui::DrawList;

    /**
     * @brief The toolbar's picture, and whether the pointer is on it.
     *
     * The one thing the tick path and the renderer have to agree on:
     * EditorSink writes it once per tick, RenderSink paints it over the
     * sheet.
     * A small shared state object rather than one asking the other, the
     * same shape as game::UiOverlay and td::ScoreOverlay -- so the
     * renderer never has to know what a pointer or a widget is.
     *
     * The canvas the bar is laid out against lives in EditorState rather
     * than here, because it is also what a click on the *image* is
     * resolved against, and one session has one of it.
     *
     * It holds nothing of its own: what goes in is described from the
     * recorded input and the state that input produced, so a replay
     * rebuilds the same picture and the same answer.
     */
    class UiOverlay final
    {
    public:
        /**
         * @brief Put this tick's UI in, replacing whatever was there.
         * @param picture The drawing commands, in the order they draw.
         * @param covered Whether the pointer is over anything the UI
         * filled in.
         */
        void set(DrawList picture, bool covered);

        /**
         * @brief Get the UI's picture.
         * @return The drawing commands, empty until the first set().
         */
        [[nodiscard]] const DrawList &commands() const noexcept;

        /**
         * @brief Check whether the UI is under the pointer.
         *
         * What keeps a click on the toolbar from also painting the pixel
         * behind it -- the sheet is drawn under the whole bar, so
         * without this every button press would leave a dot on the art.
         *
         * @return True when the pointer is over something the UI drew.
         */
        [[nodiscard]] bool pointerOverUi() const noexcept;

    private:
        DrawList picture;
        bool covered = false;
    };

} // namespace antwika::atlas_editor
