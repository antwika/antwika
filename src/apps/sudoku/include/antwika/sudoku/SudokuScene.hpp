#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/sudoku/PuzzleState.hpp"

namespace antwika::sudoku
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;

    /**
     * @brief The whole picture: a bar with a Solve button and a line of
     * feedback, and the 9x9 grid beneath it.
     *
     * Stateless and deterministic, like ui_demo::DemoScene and
     * game::Toolbar: the same canvas, pointer and PuzzleState always
     * produce the same commands in the same order and the same answer
     * about what was pressed -- which is what lets a whole frame be
     * asserted with EXPECT_EQ and no window, no mock and no backend.
     *
     * **The grid is appended to the frame the bar produced rather than
     * laid out beside it.** Its area is `ui::Frame::rects` reporting
     * where the named board container went, so the two cannot disagree
     * about how much room is left under the bar -- which is exactly the
     * drift ui::WidgetRects exists to prevent.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports: a hit-test is a
     * function of the layout, and the layout is a function of the
     * canvas, so resolving a recorded click against a differently sized
     * window would resolve it to a different square.
     * The same argument applies to the language, since a button is as
     * wide as its own label and the grid sits under whatever height the
     * bar came to -- which is why main() fixes the locale and reads one
     * from nowhere else.
     */
    class SudokuScene final
    {
    public:
        /**
         * @brief Build the scene over the translator wording it.
         * @param translator Words every label declared here. Must
         * outlive this scene.
         */
        explicit SudokuScene(const Translator &translator);

        /**
         * @brief Describe the session for one tick.
         * @param canvas The area the picture is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param state The puzzle, the selection and the last thing
         * said.
         * @return The drawing commands, what the pointer did, and where
         * every named widget was laid out.
         */
        [[nodiscard]] Frame describe(
            Size canvas, Pointer pointer, const PuzzleState &state)
            const;

        /**
         * @brief Draw one frame.
         *
         * Clears first, because this application owns the whole window:
         * there is nothing underneath for the picture to be drawn over.
         *
         * @param renderer Receives the drawing calls.
         * @param picture The commands describe() produced, by way of
         * the overlay PlaySink wrote them into.
         */
        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

} // namespace antwika::sudoku
