#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "LinesOf.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/ScrollChange.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::Frame;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;
using antwika::ui::ScrollChange;
using antwika::ui::TextAreaSpec;
using antwika::ui::Theme;
using antwika::ui::WidgetId;
using antwika::ui::tests::linesOf;

namespace
{
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kTrackColor{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumbColor{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCodeWidget{9};

    constexpr Size kCanvasSize{.width = 200, .height = 64};
    constexpr std::uint32_t kLineHeight = 8;
    constexpr std::size_t kPage = 8;
    constexpr std::uint32_t kBarWidth = 8;

    Theme plainTheme()
    {
        return Theme{
            .textColor = kInkColor,
            .scrollTrackColor = kTrackColor,
            .scrollThumbColor = kThumbColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .scrollbarWidth = kBarWidth};
    }


    [[nodiscard]] Frame frameOf(
        TextAreaSpec spec,
        const Keyboard &keyboard = {},
        const Pointer &pointer = {})
    {
        spec.widgetId = kCodeWidget;
        spec.focused = true;

        Context uiContext{kCanvasSize, plainTheme(), pointer, keyboard};

        uiContext.textArea(spec);

        return uiContext.build();
    }

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &drawList)
    {
        std::vector<std::string> texts;

        for (const auto &command : drawList)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] std::vector<FillRect> fillsOf(
        const DrawList &drawList, Color color)
    {
        std::vector<FillRect> fills;

        for (const auto &command : drawList)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == color)
            {
                fills.push_back(*fill);
            }
        }

        return fills;
    }

    [[nodiscard]] Point onTheBar(std::int32_t downPixels)
    {
        return Point{
            .x = static_cast<std::int32_t>(kCanvasSize.width - 4),
            .y = downPixels};
    }
}

TEST(TextAreaScrollTest, TextArea_ADocumentLongerThanThePaneStillDrawsItsLines)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).drawList;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "a");
    EXPECT_EQ(texts.size(), kPage);
}

TEST(TextAreaScrollTest, TextArea_APaneAsksForNoRoomOnItsDocumentsBehalf)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(
        TextAreaSpec{.widgetId = kCodeWidget, .text = linesOf(40)});

    const auto foundRect = uiContext.build().rects.find(kCodeWidget);

    ASSERT_TRUE(foundRect.has_value());
    EXPECT_EQ(foundRect->size.height, kCanvasSize.height);
}

TEST(TextAreaScrollTest, TextArea_ScrollingStartsTheDrawingFurtherDown)
{
    const auto picture =
        frameOf(
            TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3})
            .drawList;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "d");
}

TEST(TextAreaScrollTest, TextArea_ScrollingPastTheEndStillDrawsTheLastLine)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(4), .cursor = 0, .scroll = 500})
            .drawList;

    EXPECT_EQ(textsOf(picture), (std::vector<std::string>{"d"}));
}

TEST(TextAreaScrollTest, TextArea_AskingForALineTooFarDownComesBackClamped)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(20), .cursor = 0, .scroll = 500});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 20 - kPage}));
}

TEST(TextAreaScrollTest, TextArea_ADocumentShorterThanThePaneNeverScrolls)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "one\ntwo", .cursor = 0, .scroll = 5});

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 0}));
}

TEST(TextAreaScrollTest, TextArea_APaneShowingWhatWasAskedForReportsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(TextAreaScrollTest, TextArea_TypingBelowThePaneScrollsItIntoView)
{
    const auto text = linesOf(40);

    const auto frame = frameOf(
        TextAreaSpec{.text = text, .cursor = text.size(), .scroll = 0},
        Keyboard{.keys = {Key::Character}, .typedText = "X"});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_MovingTheCaretAboveThePaneScrollsItBackUp)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 20, .scroll = 20},
        Keyboard{.keys = {Key::MoveUp}});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 9U);
}

TEST(TextAreaScrollTest, TextArea_AQuietFrameLeavesThePaneWhereItIs)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 20});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnAreaWithNoBarDrawsNeitherTrackNorThumb)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).drawList;

    EXPECT_TRUE(fillsOf(picture, kTrackColor).empty());
    EXPECT_TRUE(fillsOf(picture, kThumbColor).empty());
}

TEST(TextAreaScrollTest, TextArea_ABarRunsDownTheRightHandEdge)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(40),
                    .cursor = 0,
                    .scrollbar = true})
            .drawList;

    const auto tracks = fillsOf(picture, kTrackColor);

    ASSERT_EQ(tracks.size(), 1U);
    EXPECT_EQ(tracks[0].rect.size.width, kBarWidth);
    EXPECT_EQ(tracks[0].rect.size.height, kCanvasSize.height);
    EXPECT_EQ(
        tracks[0].rect.originPoint.x,
        static_cast<std::int32_t>(kCanvasSize.width - kBarWidth));
}

TEST(TextAreaScrollTest, TextArea_TheThumbIsAsLongAsWhatIsShowing)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(32),
                    .cursor = 0,
                    .scrollbar = true})
            .drawList;

    const auto thumbs = fillsOf(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvasSize.height / 4);
    EXPECT_EQ(thumbs[0].rect.originPoint.y, 0);
}

TEST(TextAreaScrollTest, TextArea_TheThumbSitsAsFarDownAsTheTextDoes)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(32),
                    .cursor = 0,
                    .scroll = 24,
                    .scrollbar = true})
            .drawList;

    const auto thumbs = fillsOf(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);

    EXPECT_EQ(
        thumbs[0].rect.originPoint.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvasSize.height));
}

TEST(TextAreaScrollTest, TextArea_TheThumbIsNeverThinnerThanALine)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(4000),
                    .cursor = 0,
                    .scrollbar = true})
            .drawList;

    const auto thumbs = fillsOf(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kLineHeight);
}

TEST(TextAreaScrollTest, TextArea_AShortDocumentsThumbFillsTheWholeTrack)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = "one\ntwo",
                    .cursor = 0,
                    .scrollbar = true})
            .drawList;

    const auto thumbs = fillsOf(picture, kThumbColor);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvasSize.height);
}

TEST(TextAreaScrollTest, TextArea_PressingTheBarScrollsToWhereItWasPressed)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{
            .positionPoint =
                onTheBar(static_cast<std::int32_t>(kCanvasSize.height - 1)),
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_ABarPressCountsFromTheTracksOwnTop)
{
    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{
            .positionPoint = onTheBar(36), .down = true, .pressed = true}};

    uiContext.label("hd");
    uiContext.textArea(TextAreaSpec{
        .widgetId = kCodeWidget,
        .text = linesOf(40),
        .cursor = 0,
        .scrollbar = true,
        .focused = true});

    const auto frame = uiContext.build();

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 16U);
}

TEST(TextAreaScrollTest, TextArea_PressingTheTopOfTheBarScrollsBackToTheStart)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 0,
            .scroll = 20,
            .scrollbar = true},
        Keyboard{},
        Pointer{
            .positionPoint = onTheBar(0), .down = true, .pressed = true});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 0U);
}

TEST(TextAreaScrollTest, TextArea_ADragOnTheBarBeatsTheCaretItLeavesBehind)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 0,
            .scroll = 0,
            .scrollbar = true,
            .dragging = antwika::ui::DragOrigin::Track},
        Keyboard{.keys = {Key::MoveRight}},
        Pointer{
            .positionPoint =
                onTheBar(static_cast<std::int32_t>(kCanvasSize.height - 1)),
            .down = true});

    ASSERT_TRUE(frame.interactions.scrollChange.has_value());
    EXPECT_EQ(frame.interactions.scrollChange->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_ASelectionStrayingOntoTheBarMovesNoScroll)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 5,
            .scroll = 0,
            .scrollbar = true,
            .dragging = antwika::ui::DragOrigin::Text},
        Keyboard{},
        Pointer{
            .positionPoint = onTheBar(
                60), .down = true, .extendsSelection = true});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnUnnamedAreaReportsNoScroll)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{
        .text = linesOf(20), .cursor = 0, .scroll = 500});

    EXPECT_FALSE(uiContext.build().interactions.scrollChange.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnUnnamedAreaReportsNoPress)
{
    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 2}, .down = true, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.textArea(TextAreaSpec{
        .text = linesOf(20), .cursor = 0, .focused = true});

    EXPECT_FALSE(uiContext.build().interactions.areaPress.has_value());
}

TEST(TextAreaScrollTest, TextArea_APressWithNoPositionReportsNoPress)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(20), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{.down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.areaPress.has_value());
}

TEST(TextAreaScrollTest, TextArea_APressOnTheBarReportsATrackHome)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 0,
            .scrollbar = true},
        Keyboard{},
        Pointer{
            .positionPoint = onTheBar(12), .down = true, .pressed = true});

    ASSERT_TRUE(frame.interactions.areaPress.has_value());
    EXPECT_EQ(frame.interactions.areaPress->areaWidget, kCodeWidget);
    EXPECT_EQ(
        frame.interactions.areaPress->homeOrigin,
        antwika::ui::DragOrigin::Track);
}

TEST(TextAreaScrollTest, TextArea_PressingTheBarMovesNoCaret)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{
            .positionPoint = onTheBar(12), .down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaScrollTest, TextArea_RestingOverTheBarScrollsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{.positionPoint = onTheBar(60)});

    EXPECT_FALSE(frame.interactions.scrollChange.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnAreaShorterThanALineStillShowsOne)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.textArea(TextAreaSpec{
        .widgetId = kCodeWidget,
        .heightSizing = fixedSize(3),
        .text = linesOf(40),
        .scroll = 100,
        .focused = true});

    const auto frame = uiContext.build();

    EXPECT_EQ(
        frame.interactions.scrollChange,
        (ScrollChange{.areaWidget = kCodeWidget, .line = 39}));
}
