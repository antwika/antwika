#pragma once

#include <antwika/ui/WidgetId.hpp>

namespace antwika::ui_demo
{

    using antwika::ui::WidgetId;

    namespace widgets
    {
        inline constexpr WidgetId kPicker{1};

        inline constexpr WidgetId kCard{2};

        inline constexpr WidgetId kCount{3};

        inline constexpr WidgetId kReset{4};

        inline constexpr WidgetId kField{5};

        inline constexpr WidgetId kPalette{6};

        inline constexpr WidgetId kFirst{7};

        inline constexpr WidgetId kSecond{8};

        inline constexpr WidgetId kThird{9};

        inline constexpr WidgetId kMarked{10};

        inline constexpr WidgetId kSqueezed{11};

        inline constexpr WidgetId kArea{13};

        inline constexpr WidgetId kNeverDeclared{12};

        inline constexpr WidgetId kFirstPage{100};

        inline constexpr WidgetId kFirstAccent{200};
    }

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

}
