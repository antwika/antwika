#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

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

namespace
{
    constexpr Color kInk{.red = 200, .green = 210, .blue = 220};
    constexpr Color kTrack{.red = 30, .green = 33, .blue = 42};
    constexpr Color kThumb{.red = 78, .green = 86, .blue = 106};

    constexpr WidgetId kCode{9};

    // Eight lines of room at this scale, and eight pixels of bar.
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

    /**
     * @brief Get a document of one-character lines.
     * @param lines How many.
     * @return The document, with no break after the last line.
     */
    [[nodiscard]] std::string linesOf(std::size_t lines)
    {
        std::string text;

        for (std::size_t at = 0; at < lines; ++at)
        {
            text += static_cast<char>('a' + (at % 26));

            if (at + 1 < lines)
            {
                text += '\n';
            }
        }

        return text;
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

    // Somewhere on the bar, which runs down the right-hand edge.
    [[nodiscard]] Point onTheBar(std::int32_t down)
    {
        return Point{
            .x = static_cast<std::int32_t>(kCanvas.width - 4),
            .y = down};
    }
} // namespace

// A document longer than its pane used to be squeezed to nothing.
// Every line came out too short to draw a glyph in.
// It is cut off instead.
TEST(TextAreaScrollTest, ADocumentLongerThanThePaneStillDrawsItsLines)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).commands;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "a");
    EXPECT_EQ(texts.size(), kPage);
}

// And a pane holding one is as tall as it was given.
// Rather than as tall as what it holds.
TEST(TextAreaScrollTest, APaneAsksForNoRoomOnItsDocumentsBehalf)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{.id = kCode, .text = linesOf(40)});

    const auto found = ui.finish().rects.find(kCode);

    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->size.height, kCanvas.height);
}

TEST(TextAreaScrollTest, ScrollingStartsTheDrawingFurtherDown)
{
    const auto picture =
        frameOf(
            TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3})
            .commands;

    const auto texts = textsOf(picture);

    ASSERT_FALSE(texts.empty());
    EXPECT_EQ(texts.front(), "d");
}

// So a caller may add a wheel's notches to it.
// It never has to know how many lines fit.
TEST(TextAreaScrollTest, AskingForALineTooFarDownComesBackClamped)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(20), .cursor = 0, .scroll = 500});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 20 - kPage}));
}

TEST(TextAreaScrollTest, ADocumentShorterThanThePaneNeverScrolls)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = "one\ntwo", .cursor = 0, .scroll = 5});

    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 0}));
}

// A caller that stores the answer and hands it back is told nothing.
// Which is what makes it settle after one frame.
TEST(TextAreaScrollTest, APaneShowingWhatWasAskedForReportsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 3});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

// A caret that has just moved is brought into view.
// One that has not is left exactly where it is.
TEST(TextAreaScrollTest, TypingBelowThePaneScrollsItIntoView)
{
    const auto text = linesOf(40);

    const auto frame = frameOf(
        TextAreaSpec{.text = text, .cursor = text.size(), .scroll = 0},
        Keyboard{.keys = {Key::Character}, .typed = "X"});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 40 - kPage);
}

TEST(TextAreaScrollTest, MovingTheCaretAboveThePaneScrollsItBackUp)
{
    // Line ten, walking up to line nine.
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 20, .scroll = 20},
        Keyboard{.keys = {Key::MoveUp}});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 9U);
}

TEST(TextAreaScrollTest, AQuietFrameLeavesThePaneWhereItIs)
{
    const auto frame = frameOf(
        TextAreaSpec{.text = linesOf(40), .cursor = 0, .scroll = 20});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

TEST(TextAreaScrollTest, AnAreaWithNoBarDrawsNeitherTrackNorThumb)
{
    const auto picture =
        frameOf(TextAreaSpec{.text = linesOf(40), .cursor = 0}).commands;

    EXPECT_TRUE(fillsOf(picture, kTrack).empty());
    EXPECT_TRUE(fillsOf(picture, kThumb).empty());
}

TEST(TextAreaScrollTest, ABarRunsDownTheRightHandEdge)
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

// As long a share of the track as is showing of the document.
TEST(TextAreaScrollTest, TheThumbIsAsLongAsWhatIsShowing)
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

TEST(TextAreaScrollTest, TheThumbSitsAsFarDownAsTheTextDoes)
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

    // The last page, so the thumb ends where the track does.
    EXPECT_EQ(
        thumbs[0].rect.origin.y
            + static_cast<std::int32_t>(thumbs[0].rect.size.height),
        static_cast<std::int32_t>(kCanvas.height));
}

// Or a long document leaves nothing to see and nothing to grab.
TEST(TextAreaScrollTest, TheThumbIsNeverThinnerThanALine)
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

TEST(TextAreaScrollTest, AShortDocumentsThumbFillsTheWholeTrack)
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

TEST(TextAreaScrollTest, PressingTheBarScrollsToWhereItWasPressed)
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

TEST(TextAreaScrollTest, PressingTheTopOfTheBarScrollsBackToTheStart)
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

// A drag on the bar takes the text away from the caret.
// Nothing pulls it back: only a caret that just moved is followed.
TEST(TextAreaScrollTest, ADragOnTheBarBeatsTheCaretItLeavesBehind)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40),
            .cursor = 0,
            .scroll = 0,
            .scrollbar = true},
        Keyboard{.keys = {Key::MoveRight}},
        Pointer{
            .position =
                onTheBar(static_cast<std::int32_t>(kCanvas.height - 1)),
            .down = true});

    ASSERT_TRUE(frame.interactions.scrolled.has_value());
    EXPECT_EQ(frame.interactions.scrolled->line, 40 - kPage);
}

// The bar takes its width out of the room the text has.
// So a press on it is never a press on a character.
TEST(TextAreaScrollTest, PressingTheBarMovesNoCaret)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{
            .position = onTheBar(12), .down = true, .pressed = true});

    EXPECT_FALSE(frame.interactions.edit.has_value());
}

// A pointer that is not down is a pointer resting over the bar.
TEST(TextAreaScrollTest, RestingOverTheBarScrollsNothing)
{
    const auto frame = frameOf(
        TextAreaSpec{
            .text = linesOf(40), .cursor = 0, .scrollbar = true},
        Keyboard{},
        Pointer{.position = onTheBar(60)});

    EXPECT_FALSE(frame.interactions.scrolled.has_value());
}

// An area too short for a whole line still shows one.
// Otherwise everything above would be dividing by nothing.
TEST(TextAreaScrollTest, AnAreaShorterThanALineStillShowsOne)
{
    Context ui{kCanvas, plainTheme()};

    ui.textArea(TextAreaSpec{
        .id = kCode,
        .height = fixedSize(3),
        .text = linesOf(40),
        .scroll = 100,
        .focused = true});

    const auto frame = ui.finish();

    // One line showing, so the last line is the furthest it goes.
    EXPECT_EQ(
        frame.interactions.scrolled,
        (ScrollChange{.area = kCode, .line = 39}));
}
