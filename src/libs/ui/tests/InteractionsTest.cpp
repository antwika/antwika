#include <gtest/gtest.h>

#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/OptionChoice.hpp"
#include "antwika/ui/SliderChange.hpp"
#include "antwika/ui/SplitChange.hpp"
#include "antwika/ui/TextEdit.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::ui::Interactions;
using antwika::ui::OptionChoice;
using antwika::ui::TextEdit;
using antwika::ui::WidgetId;

namespace
{
    constexpr WidgetId kWidget{1};
    constexpr WidgetId kOtherWidget{2};

    constexpr Interactions kBothInteractions{
        .hoveredWidget = kWidget,
        .activatedWidget = kWidget,
        .pointerOverUi = true};

    [[nodiscard]] Interactions everything()
    {
        return Interactions{
            .hoveredWidget = kWidget,
            .activatedWidget = kOtherWidget,
            .focusedWidget = kWidget,
            .pointerOverUi = true,
            .edit =
                TextEdit{
                    .fieldWidget = kWidget,
                    .text = "ab",
                    .cursor = 1,
                    .submitted = true,
                    .cancelled = true},
            .chosenChoice =
                OptionChoice{.dropdownWidget = kOtherWidget, .index = 3}};
    }
}

TEST(InteractionsTest, OperatorEquals_MatchesWhenEveryFieldMatches)
{
    EXPECT_EQ(everything(), everything());
}

TEST(InteractionsTest, OperatorEquals_DiffersOnTheHoveredWidget)
{
    constexpr Interactions otherInteractions{
        .hoveredWidget = kOtherWidget,
        .activatedWidget = kWidget,
        .pointerOverUi = true};

    EXPECT_NE(kBothInteractions, otherInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnTheActivatedWidget)
{
    constexpr Interactions otherInteractions{
        .hoveredWidget = kWidget,
        .activatedWidget = kOtherWidget,
        .pointerOverUi = true};

    EXPECT_NE(kBothInteractions, otherInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnTheFocusedWidget)
{
    constexpr Interactions otherInteractions{
        .hoveredWidget = kWidget,
        .activatedWidget = kWidget,
        .focusedWidget = kOtherWidget,
        .pointerOverUi = true};

    EXPECT_NE(kBothInteractions, otherInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnWhereAPressLanded)
{
    const antwika::ui::TextAreaPress inText{
        .areaWidget = WidgetId{7}, .homeOrigin = antwika::ui::DragOrigin::Text};

    antwika::ui::TextAreaPress samePress = inText;
    EXPECT_EQ(inText, samePress);

    antwika::ui::TextAreaPress elsewherePress = inText;
    elsewherePress.areaWidget = WidgetId{8};
    EXPECT_NE(inText, elsewherePress);

    antwika::ui::TextAreaPress onTrackPress = inText;
    onTrackPress.homeOrigin = antwika::ui::DragOrigin::Track;
    EXPECT_NE(inText, onTrackPress);

    Interactions leftInteractions;
    Interactions rightInteractions;
    rightInteractions.areaPress = inText;
    EXPECT_NE(leftInteractions, rightInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnWhetherThePointerIsOverTheUi)
{
    constexpr Interactions otherInteractions{
        .hoveredWidget = kWidget,
        .activatedWidget = kWidget,
        .pointerOverUi = false};

    EXPECT_NE(kBothInteractions, otherInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnWhereADividerLanded)
{
    Interactions leftInteractions;
    Interactions rightInteractions;
    rightInteractions.split =
        antwika::ui::SplitChange{.dividerWidget = kWidget, .ratio = 250};

    EXPECT_NE(leftInteractions, rightInteractions);

    leftInteractions.split =
        antwika::ui::SplitChange{.dividerWidget = kWidget, .ratio = 250};

    EXPECT_EQ(leftInteractions, rightInteractions);
}

TEST(InteractionsTest, OperatorEquals_DiffersOnWhereASliderLanded)
{
    Interactions leftInteractions;
    Interactions rightInteractions;
    rightInteractions.slidChange =
        antwika::ui::SliderChange{.sliderWidget = kWidget, .value = 3};

    EXPECT_NE(leftInteractions, rightInteractions);

    leftInteractions.slidChange = antwika::ui::SliderChange{
        .sliderWidget = kWidget,
        .value = 3};

    EXPECT_EQ(leftInteractions, rightInteractions);
}
