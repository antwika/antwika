#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kWidget{1};
    constexpr WidgetId kOther{2};

    constexpr Interactions kBoth{
        .hovered = kWidget, .activated = kWidget, .pointerOverUi = true};
} // namespace

TEST(InteractionsTest, Equality_MatchesWhenEveryFieldMatches)
{
    EXPECT_EQ(kBoth, kBoth);
}

TEST(InteractionsTest, Equality_DiffersOnTheHoveredWidget)
{
    constexpr Interactions other{
        .hovered = kOther, .activated = kWidget, .pointerOverUi = true};

    EXPECT_NE(kBoth, other);
}

TEST(InteractionsTest, Equality_DiffersOnTheActivatedWidget)
{
    constexpr Interactions other{
        .hovered = kWidget, .activated = kOther, .pointerOverUi = true};

    EXPECT_NE(kBoth, other);
}

TEST(InteractionsTest, Equality_DiffersOnWhetherThePointerIsOverTheUi)
{
    constexpr Interactions other{
        .hovered = kWidget, .activated = kWidget, .pointerOverUi = false};

    EXPECT_NE(kBoth, other);
}
