#include <gtest/gtest.h>

#include "antwika/ui/HoverHint.hpp"

using antwika::ui::HoverTrack;
using antwika::ui::isTooltipDue;
using antwika::ui::kTooltipDelayFrames;
using antwika::ui::getUpdateHover;

namespace
{
    constexpr antwika::widget::WidgetId kSomeButtonWidget{80};

    constexpr antwika::widget::WidgetId kOtherButtonWidget{81};
}

TEST(HoverHintTest, UpdateHover_BeginsARestWhereTheHoverMoves)
{
    const auto track =
        getUpdateHover(HoverTrack{}, kSomeButtonWidget, 100);

    EXPECT_EQ(track.widget, kSomeButtonWidget);
    EXPECT_EQ(track.sinceFrame, 100U);
}

TEST(HoverHintTest, UpdateHover_HoldsTheRestWhereTheHoverStays)
{
    const auto begunHover =
        getUpdateHover(HoverTrack{}, kSomeButtonWidget, 100);
    const auto track = getUpdateHover(begunHover, kSomeButtonWidget, 150);

    EXPECT_EQ(track, begunHover);
}

TEST(HoverHintTest, UpdateHover_BeginsAfreshOnAnotherWidget)
{
    const auto begunHover =
        getUpdateHover(HoverTrack{}, kSomeButtonWidget, 100);
    const auto movedHover = getUpdateHover(begunHover, kOtherButtonWidget, 130);

    EXPECT_EQ(movedHover.widget, kOtherButtonWidget);
    EXPECT_EQ(movedHover.sinceFrame, 130U);
}

TEST(HoverHintTest, TooltipDue_RaisesNothingBeforeTheSecondIsUp)
{
    const auto track =
        getUpdateHover(HoverTrack{}, kSomeButtonWidget, 100);

    EXPECT_FALSE(
        isTooltipDue(track, 100 + kTooltipDelayFrames - 1));
}

TEST(HoverHintTest, TooltipDue_RaisesTheHintOnceTheSecondIsUp)
{
    const auto track =
        getUpdateHover(HoverTrack{}, kSomeButtonWidget, 100);

    EXPECT_TRUE(isTooltipDue(track, 100 + kTooltipDelayFrames));
}

TEST(HoverHintTest, TooltipDue_RaisesNothingOverNoWidgetAtAll)
{
    const auto track = getUpdateHover(
        HoverTrack{}, antwika::widget::kNoWidget, 100);

    EXPECT_FALSE(
        isTooltipDue(track, 100 + kTooltipDelayFrames + 100));
}
