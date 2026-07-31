#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::ui::OptionChoice;
using antwika::ui::TextEdit;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kWidget{1};
    constexpr WidgetId kOther{2};

    constexpr Interactions kBoth{
        .hovered = kWidget, .activated = kWidget, .pointerOverUi = true};

    // Every field set, and none of them left at its default.
    // Two of these are what make the first test below true of its name.
    // An object compared with itself passes whatever the operator does.
    [[nodiscard]] Interactions everything()
    {
        return Interactions{
            .hovered = kWidget,
            .activated = kOther,
            .focused = kWidget,
            .pointerOverUi = true,
            .edit =
                TextEdit{
                    .field = kWidget,
                    .text = "ab",
                    .cursor = 1,
                    .submitted = true,
                    .cancelled = true},
            .chosen =
                OptionChoice{.dropdown = kOther, .index = 3}};
    }
} // namespace

TEST(InteractionsTest, Equality_MatchesWhenEveryFieldMatches)
{
    EXPECT_EQ(everything(), everything());
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

TEST(InteractionsTest, Equality_DiffersOnTheFocusedWidget)
{
    constexpr Interactions other{
        .hovered = kWidget,
        .activated = kWidget,
        .focused = kOther,
        .pointerOverUi = true};

    EXPECT_NE(kBoth, other);
}

TEST(InteractionsTest, Equality_DiffersOnWhetherThePointerIsOverTheUi)
{
    constexpr Interactions other{
        .hovered = kWidget, .activated = kWidget, .pointerOverUi = false};

    EXPECT_NE(kBoth, other);
}
