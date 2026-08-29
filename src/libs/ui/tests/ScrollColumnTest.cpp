#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/support/DrawListQueries.hpp>

#include "antwika/ui/ContainerSpec.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DragOrigin.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/ScrollSpec.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::support::getFillsColored;
using antwika::ui::ContainerSpec;
using antwika::ui::Context;
using antwika::ui::DragOrigin;
using antwika::ui::DropdownSpec;
using antwika::ui::Frame;
using antwika::ui::getFixedSize;
using antwika::ui::kGrowSizing;
using antwika::ui::Pointer;
using antwika::ui::PushClip;
using antwika::ui::ScrollChange;
using antwika::ui::ScrollSpec;
using antwika::ui::Theme;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kTrackColor{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumbColor{.red = 78, .green = 86, .blue = 106};
    constexpr Color kRowColor{.red = 90, .green = 60, .blue = 60};
    constexpr Color kTopColor{.red = 60, .green = 90, .blue = 60};

    constexpr WidgetId kListWidget{7};
    constexpr WidgetId kRowWidget{11};
    constexpr WidgetId kTopWidget{13};

    constexpr Size kCanvasSize{.width = 200, .height = 64};
    constexpr std::uint32_t kRowHeight = 10;
    constexpr std::uint32_t kBarWidth = 8;
    constexpr std::uint32_t kWheelStep = 24;
    constexpr std::size_t kTenRowsOverflow = 36;

    [[nodiscard]] Theme getPlainTheme()
    {
        return Theme{
            .scrollTrackColor = kTrackColor,
            .scrollThumbColor = kThumbColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .scrollbarWidth = kBarWidth};
    }

    void addRows(Context &uiContext, const std::size_t rows)
    {
        for (std::size_t index = 0; index < rows; ++index)
        {
            const auto row = uiContext.row(
                ContainerSpec{
                    .heightSizing = getFixedSize(kRowHeight),
                    .backgroundColor = kRowColor,
                    .widgetId = index == 0 ? kRowWidget
                                           : antwika::widget::kNoWidget});
        }
    }

    [[nodiscard]] Frame frameOf(
        const std::size_t rows,
        ScrollSpec spec,
        const Pointer &pointer = {})
    {
        spec.widgetId = kListWidget;

        Context uiContext{kCanvasSize, getPlainTheme(), pointer};

        {
            const auto list = uiContext.scrollColumn(spec);

            addRows(uiContext, rows);
        }

        return uiContext.build();
    }

    [[nodiscard]] Point getOnTheBar(const std::int32_t downPixels)
    {
        return Point{
            .x = static_cast<std::int32_t>(kCanvasSize.width - 4),
            .y = downPixels};
    }
}

TEST(ScrollColumnTest, ScrollColumn_ContentShorterThanTheViewShowsNoBar)
{
    const auto picture = frameOf(3, ScrollSpec{}).drawList;

    EXPECT_TRUE(getFillsColored(picture, kTrackColor).empty());
    EXPECT_TRUE(getFillsColored(picture, kThumbColor).empty());
}

TEST(ScrollColumnTest, ScrollColumn_OverflowingContentShowsABarOnTheRight)
{
    const auto picture = frameOf(10, ScrollSpec{}).drawList;

    const auto tracks = getFillsColored(picture, kTrackColor);

    ASSERT_EQ(tracks.size(), 1U);
    EXPECT_EQ(tracks[0].rect.size.width, kBarWidth);
    EXPECT_EQ(tracks[0].rect.size.height, kCanvasSize.height);
    EXPECT_EQ(
        tracks[0].rect.originPoint.x,
        static_cast<std::int32_t>(kCanvasSize.width - kBarWidth));
}

TEST(ScrollColumnTest, ScrollColumn_OverflowingContentIsClippedToTheView)
{
    const auto picture = frameOf(10, ScrollSpec{}).drawList;

    bool clipped = false;

    for (const auto &command : picture)
    {
        const auto *clip = std::get_if<PushClip>(&command);

        if (clip != nullptr
            && clip->rect.size.height == kCanvasSize.height
            && clip->rect.size.width == kCanvasSize.width - kBarWidth)
        {
            clipped = true;
        }
    }

    EXPECT_TRUE(clipped);
}

TEST(ScrollColumnTest, ScrollColumn_TheOffsetLiftsTheChildrenUp)
{
    const auto frame = frameOf(10, ScrollSpec{.offset = 20});

    const auto foundRect = frame.rects.getWidgetRect(kRowWidget);

    ASSERT_TRUE(foundRect.has_value());
    EXPECT_EQ(foundRect->originPoint.y, -20);
    EXPECT_EQ(foundRect->size.height, kRowHeight);
}

TEST(ScrollColumnTest, ScrollColumn_ChildrenKeepTheirNaturalHeights)
{
    const auto frame = frameOf(10, ScrollSpec{});

    const auto foundRect = frame.rects.getWidgetRect(kRowWidget);

    ASSERT_TRUE(foundRect.has_value());
    EXPECT_EQ(foundRect->size.height, kRowHeight);
    EXPECT_EQ(
        foundRect->size.width, kCanvasSize.width - kBarWidth);
}

TEST(ScrollColumnTest, ScrollColumn_TheWheelScrollsDownByAStep)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = Point{.x = 50, .y = 30},
            .scrolledSteps = 1});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kListWidget, .line = kWheelStep}));
}

TEST(ScrollColumnTest, ScrollColumn_TheWheelStopsAtTheBottom)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{.offset = 30},
        Pointer{
            .positionPoint = Point{.x = 50, .y = 30},
            .scrolledSteps = 1});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{
            .areaWidget = kListWidget, .line = kTenRowsOverflow}));
}

TEST(ScrollColumnTest, ScrollColumn_TheWheelAtTheTopReportsNothing)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = Point{.x = 50, .y = 30},
            .scrolledSteps = -1});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_TheWheelAwayFromTheViewMovesNothing)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{.scrolledSteps = 1});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_AskingForTooFarDownComesBackClamped)
{
    const auto frame = frameOf(10, ScrollSpec{.offset = 500});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{
            .areaWidget = kListWidget, .line = kTenRowsOverflow}));
}

TEST(ScrollColumnTest, ScrollColumn_ShortContentComesBackAtTheTop)
{
    const auto frame = frameOf(3, ScrollSpec{.offset = 5});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kListWidget, .line = 0}));
}

TEST(ScrollColumnTest, ScrollColumn_AViewShowingWhatWasAskedForReportsNothing)
{
    const auto frame = frameOf(10, ScrollSpec{.offset = 20});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_PressingTheBarJumpsToWhereItWasPressed)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = getOnTheBar(
                static_cast<std::int32_t>(kCanvasSize.height - 1)),
            .down = true,
            .pressed = true});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{
            .areaWidget = kListWidget, .line = kTenRowsOverflow}));
}

TEST(ScrollColumnTest, ScrollColumn_APressOnTheBarReportsATrackHome)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = getOnTheBar(12),
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.areaPress.has_value());
    EXPECT_EQ(frame.interactions.areaPress->areaWidget, kListWidget);
    EXPECT_EQ(
        frame.interactions.areaPress->homeOrigin, DragOrigin::Track);
}

TEST(ScrollColumnTest, ScrollColumn_ADragOnTheBarKeepsFollowingThePointer)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{.dragging = true},
        Pointer{.positionPoint = getOnTheBar(31), .down = true});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(
        frame.interactions.scrollChange->line,
        static_cast<std::size_t>(31) * kTenRowsOverflow
            / (kCanvasSize.height - 1));
}

TEST(ScrollColumnTest, ScrollColumn_TheThumbShowsTheVisibleShare)
{
    const auto picture = frameOf(10, ScrollSpec{}).drawList;

    const auto thumbs = getFillsColored(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(
        thumbs[0].rect.size.height,
        kCanvasSize.height * kCanvasSize.height
            / (10 * kRowHeight));
    EXPECT_EQ(thumbs[0].rect.originPoint.y, 0);
}

TEST(ScrollColumnTest, ScrollColumn_TheThumbRidesDownWithTheOffset)
{
    const auto picture =
        frameOf(10, ScrollSpec{.offset = kTenRowsOverflow}).drawList;

    const auto thumbs = getFillsColored(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(
        thumbs[0].rect.originPoint.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvasSize.height));
}

TEST(ScrollColumnTest, ScrollColumn_AScrolledAwayChildTakesNoHover)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{.positionPoint = Point{.x = 50, .y = 10}}};

    {
        const auto top = uiContext.row(
            ContainerSpec{
                .heightSizing = getFixedSize(20),
                .backgroundColor = kTopColor,
                .widgetId = kTopWidget});
    }

    {
        const auto list = uiContext.scrollColumn(
            ScrollSpec{.widgetId = kListWidget, .offset = 20});

        addRows(uiContext, 10);
    }

    const auto frame = uiContext.build();

    EXPECT_EQ(frame.interactions.hoveredWidget, kTopWidget);
}

TEST(ScrollColumnTest, ScrollColumn_AnUnnamedViewReportsNoScroll)
{
    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = getOnTheBar(12),
            .down = true,
            .pressed = true}};

    {
        const auto list = uiContext.scrollColumn(ScrollSpec{.offset = 500});

        addRows(uiContext, 10);
    }

    const auto frame = uiContext.build();

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
    EXPECT_FALSE(frame.interactions.areaPress.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_PressingTheTopOfTheBarScrollsToTheStart)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{.offset = 20},
        Pointer{
            .positionPoint = getOnTheBar(0),
            .down = true,
            .pressed = true});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kListWidget, .line = 0}));
}

TEST(ScrollColumnTest, ScrollColumn_APressOnAShortViewReportsNoTrackHome)
{
    const auto frame = frameOf(
        3,
        ScrollSpec{},
        Pointer{
            .positionPoint = Point{.x = 50, .y = 10},
            .down = true,
            .pressed = true});

    EXPECT_FALSE(frame.interactions.areaPress.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_APressOnTheContentReportsNoTrackHome)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = Point{.x = 50, .y = 30},
            .down = true,
            .pressed = true});

    EXPECT_FALSE(frame.interactions.areaPress.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_APressWithNoPositionReportsNoTrackHome)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{.down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.areaPress.has_value());
    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_TheWheelOverTheBarMovesNothing)
{
    const auto frame = frameOf(
        10,
        ScrollSpec{},
        Pointer{
            .positionPoint = getOnTheBar(31),
            .scrolledSteps = 1});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(ScrollColumnTest, ScrollColumn_AnEmptyViewComesBackAtTheTop)
{
    const auto frame = frameOf(0, ScrollSpec{.offset = 5});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kListWidget, .line = 0}));
}

TEST(ScrollColumnTest, ScrollColumn_AViewOfFlatRowsHasNothingToScroll)
{
    ScrollSpec spec{.widgetId = kListWidget, .offset = 5};

    Context uiContext{kCanvasSize, getPlainTheme()};

    {
        const auto list = uiContext.scrollColumn(spec);

        const auto row = uiContext.row(
            ContainerSpec{.heightSizing = getFixedSize(0)});
    }

    const auto frame = uiContext.build();

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kListWidget, .line = 0}));
}

TEST(ScrollColumnTest, ScrollColumn_AnOverlayOverTheViewTakesTheWheel)
{
    const std::array<std::string_view, 1> options{"one"};

    Context uiContext{
        kCanvasSize,
        getPlainTheme(),
        Pointer{
            .positionPoint = Point{.x = 10, .y = 12},
            .down = true,
            .pressed = true,
            .scrolledSteps = 1}};

    {
        const auto list = uiContext.scrollColumn(
            ScrollSpec{.widgetId = kListWidget});

        uiContext.dropdown(
            DropdownSpec{
                .widgetId = kTopWidget,
                .options = options,
                .placeholder = "pick",
                .open = true});

        addRows(uiContext, 10);
    }

    const auto frame = uiContext.build();

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
    EXPECT_FALSE(frame.interactions.areaPress.has_value());
}
