#pragma once

#include <antwika/ui/WidgetId.hpp>

namespace antwika::sudoku
{

    using antwika::ui::WidgetId;

    /**
     * @brief What this application's widgets are called.
     *
     * Symbolic names rather than where a widget ended up in the layout,
     * because this is what crosses back into application state.
     * None of them ever reaches a replay: what is recorded is the click
     * or the keystroke, and which widget it hit is worked out again
     * from it -- see PlaySink, and the rule that no `ui.*` event name
     * may ever exist.
     */
    namespace widgets
    {
        /** @brief Finishes the grid from wherever it has got to. */
        inline constexpr WidgetId kSolve{1};

        /**
         * @brief The area the 9x9 grid is drawn into.
         *
         * A named container with no fill of its own, so the layout
         * reports where it went and the grid is placed *from* that
         * rectangle rather than beside it.
         * Naming it is also what makes a press on the grid arrive as an
         * activation, so one hit-test decides whether a click was the
         * bar's or the board's.
         */
        inline constexpr WidgetId kBoard{2};
    } // namespace widgets

    // Two widgets sharing an id would be one widget, silently.
    // These are constants, so the mistake can be a build error instead.
    static_assert(
        antwika::ui::assertDistinct(widgets::kSolve, widgets::kBoard),
        "every widget in this application needs its own id");

} // namespace antwika::sudoku
