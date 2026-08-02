#pragma once

#include <antwika/ui/WidgetId.hpp>

namespace antwika::ui_demo
{

    using antwika::ui::WidgetId;

    /**
     * @brief What this demo's widgets are called.
     *
     * Symbolic names rather than where a widget ended up in the layout,
     * because this is what crosses back into application state.
     * None of them ever reaches a replay: what is recorded is the click
     * or the keystroke, and which widget it hit is worked out again from
     * it -- see DemoSink.
     */
    namespace widgets
    {
        /** @brief The list that chooses which page is shown. */
        inline constexpr WidgetId kPicker{1};

        /** @brief The page's own panel, named so its area is reported. */
        inline constexpr WidgetId kCard{2};

        /** @brief Adds one to the click counter. */
        inline constexpr WidgetId kCount{3};

        /** @brief Puts the click counter back to zero. */
        inline constexpr WidgetId kReset{4};

        /** @brief The field the demo's characters are typed into. */
        inline constexpr WidgetId kField{5};

        /** @brief The second list, the one that picks an accent. */
        inline constexpr WidgetId kPalette{6};

        /** @brief The first of three buttons Tab walks between. */
        inline constexpr WidgetId kFirst{7};

        /** @brief The second of them. */
        inline constexpr WidgetId kSecond{8};

        /** @brief The third of them. */
        inline constexpr WidgetId kThird{9};

        /** @brief The row the widget-rects page marks out. */
        inline constexpr WidgetId kMarked{10};

        /** @brief The row whose children have less room than they ask. */
        inline constexpr WidgetId kSqueezed{11};

        /** @brief The many-line pane the text-area page shows. */
        inline constexpr WidgetId kArea{13};

        /**
         * @brief An id no frame ever declares.
         *
         * Here so the demo can show what Frame::rects answers for one:
         * nothing, rather than a rectangle of its own invention.
         */
        inline constexpr WidgetId kNeverDeclared{12};

        /**
         * @brief The picker's first option; option n carries this plus n.
         *
         * Far above the widgets beside it, so the two ranges cannot meet
         * however many pages the showcase grows.
         */
        inline constexpr WidgetId kFirstPage{100};

        /** @brief The accent list's first option, on the same terms. */
        inline constexpr WidgetId kFirstAccent{200};
    } // namespace widgets

    // Two widgets sharing an id would be one widget, silently.
    // These are constants, so the mistake can be a build error instead.
    static_assert(
        antwika::ui::assertDistinct(
            widgets::kPicker,
            widgets::kCard,
            widgets::kCount,
            widgets::kReset,
            widgets::kField,
            widgets::kPalette,
            widgets::kFirst,
            widgets::kSecond,
            widgets::kThird,
            widgets::kMarked,
            widgets::kSqueezed,
            widgets::kNeverDeclared,
            widgets::kArea,
            widgets::kFirstPage,
            widgets::kFirstAccent),
        "every widget in this demo needs its own id");

} // namespace antwika::ui_demo
