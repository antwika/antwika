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
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kTrack{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumb{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCode{9};

    constexpr Size kCanvas{.width = 200, .height = 64};
    constexpr std::uint32_t kLineHeight = 8;
    constexpr std::size_t kPage = 8;
    constexpr std::uint32_t kBarWidth = 8;

    Theme plainTheme()
    {
        return Theme{
            .text = kInk,
            .scrollTrack = kTrack,
            .scrollThumb = kThumb,
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
        spec.id = kCode;
        spec.focused = true;

        Context ui{kCanvas, plainTheme(), pointer, keyboard};

        ui.textArea(spec);

        return ui.finish();
    }

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] std::vector<FillRect> fillsOf(
        const DrawList &commands, Color color)
    {
        std::vector<FillRect> fills;

        for (const auto &command : commands)
        {
            const auto *fill = std::get_if<FillRect>(&command);

            if (fill != nullptr && fill->color == color)
            {
                fills.push_back(*fill);
            }
        }

        return fills;
    }

    [[nodiscard]] Point onTheBar(std::int32_t down)
    {
        return Point{
            .x = static_cast<std::int32_t>(kCanvas.width - 4),
            .y = down};
    }
}

TEST(TextAreaScrollTest, TextArea_ADocumentLongerThanThePaneStillDrawsItsLines)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).commands;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "a");
    EXPECT_EQ(texts.size(), kPage);
}

TEST(TextAreaScrollTest, TextArea_APaneAsksForNoRoomOnItsDocumentsBehalf)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{.id = kCode, .text = linesOf(40)});

    const auto found = ui.finish().rects.find(kCode);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->size.height, kCanvas.height);
}

TEST(TextAreaScrollTest, TextArea_ScrollingStartsTheDrawingFurtherDown)
{
    const auto picture =
        frameOf(
            TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3})
            .commands;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "d");
}

TEST(TextAreaScrollTest, TextArea_ScrollingPastTheEndStillDrawsTheLastLine)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(4), .cursor = 0, .scroll = 500})
            .commands;

    EXPECT_EQ(textsOf(picture), (std::vector<std::string>{"d"}));
}

TEST(TextAreaScrollTest, TextArea_AskingForALineTooFarDownComesBackClamped)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(20), .cursor = 0, .scroll = 500});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 20 - kPage}));
}

TEST(TextAreaScrollTest, TextArea_ADocumentShorterThanThePaneNeverScrolls)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "one\ntwo", .cursor = 0, .scroll = 5});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 0}));
}

TEST(TextAreaScrollTest, TextArea_APaneShowingWhatWasAskedForReportsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, TextArea_TypingBelowThePaneScrollsItIntoView)
{
    const auto text = linesOf(40);

    const auto frame = frameOf(
        TextAreaSpec{.text = text, .cursor = text.size(), .scroll = 0},
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_MovingTheCaretAboveThePaneScrollsItBackUp)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 20, .scroll = 20},
        Keyboard{.keys = {Key::MoveUp}});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 9U);
}

TEST(TextAreaScrollTest, TextArea_AQuietFrameLeavesThePaneWhereItIs)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 20});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnAreaWithNoBarDrawsNeitherTrackNorThumb)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).commands;

    EXPECT_TRUE(fillsOf(picture, kTrack).empty());
    EXPECT_TRUE(fillsOf(picture, kThumb).empty());
}

TEST(TextAreaScrollTest, TextArea_ABarRunsDownTheRightHandEdge)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(40),
                    .cursor = 0,
                    .scrollbar = true})
            .commands;

    const auto tracks = fillsOf(picture, kTrack);

    ASSERT_EQ(tracks.size(), 1U);
    EXPECT_EQ(tracks[0].rect.size.width, kBarWidth);
    EXPECT_EQ(tracks[0].rect.size.height, kCanvas.height);
    EXPECT_EQ(
        tracks[0].rect.origin.x,
        static_cast<std::int32_t>(kCanvas.width - kBarWidth));
}

TEST(TextAreaScrollTest, TextArea_TheThumbIsAsLongAsWhatIsShowing)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(32),
                    .cursor = 0,
                    .scrollbar = true})
            .commands;

    const auto thumbs = fillsOf(picture, kThumb);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvas.height / 4);
    EXPECT_EQ(thumbs[0].rect.origin.y, 0);
}

TEST(TextAreaScrollTest, TextArea_TheThumbSitsAsFarDownAsTheTextDoes)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(32),
                    .cursor = 0,
                    .scroll = 24,
                    .scrollbar = true})
            .commands;

    const auto thumbs = fillsOf(picture, kThumb);

    ASSERT_EQ(thumbs.size(), 1U);

    EXPECT_EQ(
        thumbs[0].rect.origin.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvas.height));
}

TEST(TextAreaScrollTest, TextArea_TheThumbIsNeverThinnerThanALine)
{
    const auto picture =
        frameOf(TextAreaSpec{
                    .text = linesOf(4000),
                    .cursor = 0,
                    .scrollbar = true})
            .commands;

    const auto thumbs = fillsOf(picture, kThumb);

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
            .commands;

    const auto thumbs = fillsOf(picture, kThumb);

    ASSERT_EQ(thumbs.size(), 1U);
    EXPECT_EQ(thumbs[0].rect.size.height, kCanvas.height);
}

TEST(TextAreaScrollTest, TextArea_PressingTheBarScrollsToWhereItWasPressed)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{
            .position =
                onTheBar(static_cast<std::int32_t>(kCanvas.height - 1)),
            .down = true,
            .pressed = true});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_ABarPressCountsFromTheTracksOwnTop)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{
            .position = onTheBar(36), .down = true, .pressed = true}};

    ui.label("hd");
    ui.textArea(TextAreaSpec{
        .id = kCode,
        .text = linesOf(40),
        .cursor = 0,
        .scrollbar = true,
        .focused = true});

    const auto frame = ui.finish();

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 16U);
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
            .position = onTheBar(0), .down = true, .pressed = true});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 0U);
}

TEST(TextAreaScrollTest, TextArea_ADragOnTheBarBeatsTheCaretItLeavesBehind)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 0,
            .scroll = 0,
            .scrollbar = true,
            .dragging = antwika::ui::DragHome::Track},
        Keyboard{.keys = {Key::MoveRight}},
        Pointer{
            .position =
                onTheBar(static_cast<std::int32_t>(kCanvas.height - 1)),
            .down = true});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 40 - kPage);
}

TEST(TextAreaScrollTest, TextArea_ASelectionStrayingOntoTheBarMovesNoScroll)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 5,
            .scroll = 0,
            .scrollbar = true,
            .dragging = antwika::ui::DragHome::Text},
        Keyboard{},
        Pointer{
            .position = onTheBar(60), .down = true, .extends = true});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnUnnamedAreaReportsNoScroll)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{
        .text = linesOf(20), .cursor = 0, .scroll = 500});

    EXPECT_FALSE(ui.finish().interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnUnnamedAreaReportsNoPress)
{
    const Pointer pointer{
        .position = Point{.x = 2, .y = 2}, .down = true, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.textArea(TextAreaSpec{
        .text = linesOf(20), .cursor = 0, .focused = true});

    EXPECT_FALSE(ui.finish().interactions.areaPress.has_value());
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
            .position = onTheBar(12), .down = true, .pressed = true});

    ASSERT_TRUE(frame.interactions.areaPress.has_value());
    EXPECT_EQ(frame.interactions.areaPress->area, kCode);
    EXPECT_EQ(
        frame.interactions.areaPress->home,
        antwika::ui::DragHome::Track);
}

TEST(TextAreaScrollTest, TextArea_PressingTheBarMovesNoCaret)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{
            .position = onTheBar(12), .down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

TEST(TextAreaScrollTest, TextArea_RestingOverTheBarScrollsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{.position = onTheBar(60)});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, TextArea_AnAreaShorterThanALineStillShowsOne)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{
        .id = kCode,
        .height = fixedSize(3),
        .text = linesOf(40),
        .scroll = 100,
        .focused = true});

    const auto frame = ui.finish();

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 39}));
}
