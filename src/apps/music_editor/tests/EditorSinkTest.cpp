#include "antwika/music_editor/EditorSink.hpp"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "EditorRig.hpp"

using antwika::event::TickEvent;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::music_editor::kCodeField;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::tests::EditorRig;
using antwika::music_editor::tests::tickAt;
using antwika::music_editor::tests::tickThrough;

namespace
{
    using antwika::input::KeyModifiers;

    constexpr KeyModifiers kShift{.shift = true};
    constexpr KeyModifiers kControl{.control = true};
    constexpr KeyModifiers kAlt{.alt = true};

    // Types one key, exactly as a recording would have held it.
    void press(
        EditorRig &rig,
        Key key,
        antwika::time::Tick when,
        KeyModifiers modifiers = {})
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    KeyPressed{.key = key, .modifiers = modifiers})});
    }

    void clickAt(
        EditorRig &rig,
        antwika::gfx::Point at,
        antwika::time::Tick when,
        KeyModifiers modifiers = {})
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = {.x = at.x, .y = at.y},
                        .modifiers = modifiers})});
    }

    void moveTo(
        EditorRig &rig, antwika::gfx::Point at, antwika::time::Tick when)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(antwika::input::PointerMoved{
                    .position = {.x = at.x, .y = at.y}})});
    }

    void releaseAt(
        EditorRig &rig, antwika::gfx::Point at, antwika::time::Tick when)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    antwika::input::PointerButtonReleased{
                        .button = MouseButton::Left,
                        .position = {.x = at.x, .y = at.y}})});
    }

    void wheel(
        EditorRig &rig,
        std::int32_t notches,
        antwika::time::Tick when)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    antwika::input::PointerScrolled{
                        .vertical = notches})});
    }

    [[nodiscard]] antwika::gfx::Rect rectOf(
        EditorRig &rig, antwika::ui::WidgetId id)
    {
        const auto frame = rig.scene.describe(
            rig.state,
            rig.score,
            antwika::music_editor::PlaybackStatus{},
            antwika::music_editor::tests::kCanvas,
            {},
            {});

        const auto found = frame.rects.find(id);
        EXPECT_TRUE(found.has_value());

        return found.value_or(antwika::gfx::Rect{});
    }

    // Where one line and one glyph cell of the pane is.
    // So a test clicks a character rather than a pixel it worked out.
    // The pane insets its text by the theme's button padding.
    // Every cell is the glyph's size times the editor's scale.
    [[nodiscard]] antwika::gfx::Point cellOf(
        EditorRig &rig, std::int32_t line, std::int32_t column)
    {
        const auto pane = rectOf(rig, kCodeField);
        const auto theme = antwika::music_editor::editorTheme();

        const auto inset = static_cast<std::int32_t>(theme.buttonPadding);
        const auto advance = static_cast<std::int32_t>(
            antwika::gfx::kGlyphAdvance * theme.textScale);
        const auto height = static_cast<std::int32_t>(
            antwika::gfx::kGlyphLineHeight * theme.textScale);

        // Inside the cell rather than on its corner.
        // A corner belongs to whichever cell was put before it.
        return antwika::gfx::Point{
            .x = pane.origin.x + inset + column * advance + 1,
            .y = pane.origin.y + inset + line * height + 1};
    }

    [[nodiscard]] antwika::gfx::Point centreOf(
        EditorRig &rig, antwika::ui::WidgetId id)
    {
        const auto frame = rig.scene.describe(
            rig.state,
            rig.score,
            antwika::music_editor::PlaybackStatus{},
            antwika::music_editor::tests::kCanvas,
            {},
            {});

        const auto found = frame.rects.find(id);
        EXPECT_TRUE(found.has_value());

        const auto rect = found.value_or(antwika::gfx::Rect{});

        // The centre rather than a corner.
        // A corner belongs to whatever the layout put beside it.
        return antwika::gfx::Point{
            .x = rect.origin.x
                + static_cast<std::int32_t>(rect.size.width) / 2,
            .y = rect.origin.y
                + static_cast<std::int32_t>(rect.size.height) / 2};
    }
} // namespace

TEST(EditorSinkTest, DrawsSomethingOnTheVeryFirstTick)
{
    EditorRig rig;

    rig.editor.handle(tickAt(0));

    EXPECT_FALSE(rig.editor.commands().empty());
}

// The document is re-read from what the input just did.
// Only then is the sound advanced.
TEST(EditorSinkTest, KeepsPlayingWithoutAnybodyStartingIt)
{
    EditorRig rig;

    tickThrough(rig, 0, 20);

    EXPECT_GT(rig.playback.started(), 0U);
    EXPECT_EQ(rig.playback.playedTicks(), 20U);
}

// This app defines no event of its own.
// A keystroke is recorded, and where it landed is regenerated.
TEST(EditorSinkTest, TypingGoesIntoTheDocumentAtTheCaret)
{
    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit5, 1);

    EXPECT_EQ(rig.state.source, "5");
    EXPECT_EQ(rig.state.cursor, 1U);
}

// A caret in the middle is what typing into a written score is.
TEST(EditorSinkTest, TypingIntoTheMiddleLeavesTheRestWhereItWas)
{
    EditorRig rig;
    rig.state.source = "$: bass.n(\"0\")\n$: lead.n(\"3\")\n";
    rig.state.cursor = 12;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit7, 1);

    EXPECT_EQ(
        rig.state.source, "$: bass.n(\"07\")\n$: lead.n(\"3\")\n");
    EXPECT_EQ(rig.state.cursor, 13U);
}

// One tick's typing is one frame's worth of edges, in arrival order.
TEST(EditorSinkTest, TabIndentsByTwoRatherThanMovingAnything)
{
    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Tab, 1);

    EXPECT_EQ(rig.state.source, "  ");
    EXPECT_EQ(rig.state.cursor, 2U);
}

TEST(EditorSinkTest, BackspaceTakesTheCharacterBeforeTheCaret)
{
    EditorRig rig;
    rig.state.source = "0 3";
    rig.state.cursor = 3;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Backspace, 1);

    EXPECT_EQ(rig.state.source, "0 ");
}

// Enter writes a line break, which is how a second voice line begins.
// It is deliberately not the pause any more.
TEST(EditorSinkTest, EnterIsANewLineRatherThanAPause)
{
    EditorRig rig;
    rig.state.source = "$: bass.n(\"0\")";
    rig.state.cursor = 14;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);

    EXPECT_EQ(rig.state.source, "$: bass.n(\"0\")\n");
    EXPECT_FALSE(rig.state.paused);
}

TEST(EditorSinkTest, TheArrowsWalkTheCaretBetweenLines)
{
    EditorRig rig;
    rig.state.source = "abcd\nefgh\nijkl";
    rig.state.cursor = 12;

    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowUp, 1);
    EXPECT_EQ(rig.state.cursor, 7U);

    press(rig, Key::ArrowUp, 2);
    EXPECT_EQ(rig.state.cursor, 2U);

    press(rig, Key::ArrowDown, 3);
    EXPECT_EQ(rig.state.cursor, 7U);

    press(rig, Key::ArrowLeft, 4);
    EXPECT_EQ(rig.state.cursor, 6U);

    press(rig, Key::ArrowRight, 5);
    EXPECT_EQ(rig.state.cursor, 7U);
}

// Uncommenting a line is all it takes for that line to sound.
TEST(EditorSinkTest, TypingReachesTheSoundWithoutAnythingBeingReloaded)
{
    EditorRig rig;
    rig.state.source = "// $: bass.n(\"0*4\")\n";
    rig.state.cursor = 2;

    tickThrough(rig, 0, 4);

    ASSERT_EQ(rig.playback.started(), 0U);
    ASSERT_EQ(rig.playback.sounding(), 0U);

    press(rig, Key::Backspace, 5);
    press(rig, Key::Backspace, 5);

    tickThrough(rig, 6, 30);

    EXPECT_EQ(rig.state.source, " $: bass.n(\"0*4\")\n");
    EXPECT_EQ(rig.playback.sounding(), 1U);
    EXPECT_GT(rig.playback.started(), 0U);
}

// Escape is this application's alone.
// A field told about it would give up on what was typed.
TEST(EditorSinkTest, EscapePausesAndResumes)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto written = rig.state.source;

    press(rig, Key::Escape, 1);
    EXPECT_TRUE(rig.state.paused);

    press(rig, Key::Escape, 2);
    EXPECT_FALSE(rig.state.paused);

    EXPECT_EQ(rig.state.source, written);
}

TEST(EditorSinkTest, PausingStopsTheMusicalClock)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Escape, 1);
    ASSERT_TRUE(rig.state.paused);

    const auto played = rig.playback.playedTicks();

    tickThrough(rig, 2, 10);

    EXPECT_EQ(rig.playback.playedTicks(), played);
}

TEST(EditorSinkTest, TheButtonPausesToo)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPlayButton), 1);

    EXPECT_TRUE(rig.state.paused);
}

TEST(EditorSinkTest, TheOtherButtonSilencesEveryVoice)
{
    EditorRig rig;
    rig.state.source = "$: bell.n(\"0*8\")\n";

    tickThrough(rig, 0, 6);

    ASSERT_GT(rig.mixer.activeVoices(), 0U);

    clickAt(rig, centreOf(rig, kPanicButton), 7);

    EXPECT_EQ(rig.mixer.activeVoices(), 0U);
    EXPECT_FALSE(rig.state.paused);
}

// Clicking past the last line is the end of the document.
// So clicking the empty part of the pane carries on writing.
TEST(EditorSinkTest, ClickingBelowTheTextPutsTheCaretAtTheEnd)
{
    EditorRig rig;
    rig.state.source = "0 3";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kCodeField), 1);

    EXPECT_EQ(rig.state.cursor, 3U);

    press(rig, Key::Digit5, 2);

    EXPECT_EQ(rig.state.source, "0 35");
}

// The pane is a grid of whole cells.
// So which character was clicked is arithmetic, not a measurement.
TEST(EditorSinkTest, ClickingACharacterPutsTheCaretOnIt)
{
    EditorRig rig;
    rig.state.source = "abcd\nefgh";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 1, 2), 1);

    // Both ends together, since a click selects nothing.
    EXPECT_EQ(rig.state.cursor, 7U);
    EXPECT_EQ(rig.state.anchor, 7U);

    press(rig, Key::Digit5, 2);

    EXPECT_EQ(rig.state.source, "abcd\nef5gh");
}

// Which end is the caret decides where the next shift-click goes.
// So both of them are kept.
TEST(EditorSinkTest, ShiftClickingCarriesTheSelectionToWhereItLands)
{
    EditorRig rig;
    rig.state.source = "abcd\nefgh";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 1), 1);
    clickAt(rig, cellOf(rig, 1, 3), 2, kShift);

    EXPECT_EQ(rig.state.cursor, 8U);
    EXPECT_EQ(rig.state.anchor, 1U);

    press(rig, Key::C, 3, kControl);

    EXPECT_EQ(rig.state.clipboard, "bcd\nefg");
}

// A press and then a move with the button still down.
TEST(EditorSinkTest, DraggingAcrossThePaneSelectsWhatItCrossed)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 0), 1);
    moveTo(rig, cellOf(rig, 0, 3), 1);

    EXPECT_EQ(rig.state.cursor, 3U);
    EXPECT_EQ(rig.state.anchor, 0U);

    // And the drag ends where the button comes back up.
    releaseAt(rig, cellOf(rig, 0, 3), 1);
    moveTo(rig, cellOf(rig, 0, 1), 2);

    EXPECT_EQ(rig.state.cursor, 3U);
}

// Only a move drags.
// So a key pressed with the button held leaves the caret alone.
TEST(EditorSinkTest, TypingMidDragLeavesTheCaretWhereItTypes)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 3), 1);
    ASSERT_EQ(rig.state.cursor, 3U);

    press(rig, Key::Digit5, 2);

    EXPECT_EQ(rig.state.source, "abc5d");
    EXPECT_EQ(rig.state.cursor, 4U);
}

// A drag that began on a button is not a drag through the pane.
TEST(EditorSinkTest, AMoveWithNoPressBehindItSelectsNothing)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPanicButton), 1);
    moveTo(rig, cellOf(rig, 0, 3), 2);

    EXPECT_EQ(rig.state.cursor, 0U);
}

TEST(EditorSinkTest, AnEventItCannotDecodeChangesNothing)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto before = rig.state;

    rig.editor.handle(
        TickEvent{.tick = 1, .event = {.name = "something.else"}});

    EXPECT_EQ(rig.state, before);
}

// A line that will not parse keeps playing whatever it last did.
// So half a bracket does not silence the editor.
TEST(EditorSinkTest, ARefusedLineIsReportedAndKeepsSounding)
{
    EditorRig rig;
    rig.state.source = "$: bass.n(\"0 3\")\n";
    rig.state.cursor = 14;

    tickThrough(rig, 0, 4);

    const auto sounded = rig.playback.started();

    // Which is where a Swedish board keeps its opening bracket.
    press(rig, Key::Digit8, 5, kAlt);
    rig.editor.handle(tickAt(6));

    EXPECT_EQ(rig.state.source, "$: bass.n(\"0 3[\")\n");
    EXPECT_TRUE(rig.score.hasError());

    tickThrough(rig, 7, 40);

    EXPECT_GT(rig.playback.started(), sounded);
}

// A drag is the left button's, so a release of any other leaves it.
TEST(EditorSinkTest, ARightReleaseEndsNoDrag)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 0), 1);
    ASSERT_TRUE(rig.state.dragging);

    rig.editor.handle(
        TickEvent{
            .tick = 2,
            .event = rig.codec.encode(
                antwika::input::PointerButtonReleased{
                    .button = MouseButton::Right,
                    .position = {.x = 0, .y = 0}})});

    EXPECT_TRUE(rig.state.dragging);
}

// Only a left press reaches the layout.
TEST(EditorSinkTest, ARightPressPressesNothing)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto at = centreOf(rig, kPlayButton);

    rig.editor.handle(
        TickEvent{
            .tick = 1,
            .event = rig.codec.encode(
                PointerButtonPressed{
                    .button = MouseButton::Right,
                    .position = {.x = at.x, .y = at.y}})});

    EXPECT_FALSE(rig.state.paused);
}

// The pointer stays located once it has been anywhere.
// So a second press resolves without being told the position again.
TEST(EditorSinkTest, RemembersThatThePointerHasBeenSomewhere)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPanicButton), 1);

    clickAt(rig, centreOf(rig, kPlayButton), 2);

    EXPECT_TRUE(rig.state.paused);
}

// A tick carries no keys and no press.
// So nothing is activated and nothing is typed.
TEST(EditorSinkTest, ATickOnItsOwnActivatesNothing)
{
    EditorRig rig;

    rig.editor.handle(tickAt(0));
    const auto before = rig.state;

    rig.editor.handle(tickAt(1));

    EXPECT_EQ(rig.state, before);
}

// A key meaning nothing and typing nothing leaves the document be.
TEST(EditorSinkTest, AKeyThatSaysNothingChangesNothing)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto before = rig.state;

    press(rig, Key::F1, 1);

    EXPECT_EQ(rig.state, before);
}

TEST(EditorSinkTest, ShiftAndAnArrowSelectsWithoutMovingTheFarEnd)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 1;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);

    EXPECT_EQ(rig.state.cursor, 3U);
    EXPECT_EQ(rig.state.anchor, 1U);

    // And typing over a selection takes the whole of it.
    press(rig, Key::Digit5, 3);

    EXPECT_EQ(rig.state.source, "a5d");
}

// The clipboard is this editor's own.
// So a replay pastes what the run pasted rather than what it holds.
TEST(EditorSinkTest, CopiesAndPastesThroughItsOwnClipboard)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);
    press(rig, Key::C, 3, kControl);

    EXPECT_EQ(rig.state.clipboard, "ab");
    EXPECT_EQ(rig.state.source, "abcd");

    press(rig, Key::ArrowRight, 4);
    press(rig, Key::ArrowRight, 5);
    press(rig, Key::ArrowRight, 6);
    press(rig, Key::V, 7, kControl);

    EXPECT_EQ(rig.state.source, "abcdab");
    EXPECT_EQ(rig.state.cursor, 6U);
}

TEST(EditorSinkTest, CuttingTakesTheSelectionWithIt)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 2;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);
    press(rig, Key::X, 3, kControl);

    EXPECT_EQ(rig.state.source, "ab");
    EXPECT_EQ(rig.state.clipboard, "cd");

    press(rig, Key::V, 4, kControl);
    press(rig, Key::V, 5, kControl);

    EXPECT_EQ(rig.state.source, "abcdcd");
}

// A copy with nothing selected must not empty what was copied before.
TEST(EditorSinkTest, CopyingNothingKeepsWhatWasCopiedBefore)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::C, 2, kControl);

    ASSERT_EQ(rig.state.clipboard, "a");

    press(rig, Key::ArrowRight, 3);
    press(rig, Key::C, 4, kControl);

    EXPECT_EQ(rig.state.clipboard, "a");
}

// Delete is Backspace's other half, and takes a whole selection too.
TEST(EditorSinkTest, DeleteTakesTheCharacterTheCaretSitsBefore)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 1;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Delete, 1);

    EXPECT_EQ(rig.state.source, "acd");
}

// How far a pane can usefully be scrolled is antwika::ui's answer.
// So a wheel asking for more comes back clamped.
TEST(EditorSinkTest, TheWheelShiftsThePaneAndStopsAtTheEnds)
{
    EditorRig rig;
    rig.state.source = std::string(200, '\n');
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    wheel(rig, -1, 1);

    EXPECT_EQ(rig.state.scroll, 3U);

    wheel(rig, 1, 2);

    EXPECT_EQ(rig.state.scroll, 0U);

    // Past the top is the top.
    wheel(rig, 4, 3);

    EXPECT_EQ(rig.state.scroll, 0U);

    // And past the end is as far as the last page.
    wheel(rig, -300, 4);

    EXPECT_LT(rig.state.scroll, 200U);
    EXPECT_GT(rig.state.scroll, 0U);
}

// The caret is brought into view by the pane, not by anything here.
// This is that arriving.
TEST(EditorSinkTest, TypingBelowThePaneScrollsItIntoView)
{
    EditorRig rig;
    rig.state.source = std::string(200, '\n');
    rig.state.cursor = rig.state.source.size();
    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit5, 1);

    EXPECT_GT(rig.state.scroll, 100U);
}

// The one setting this editor has.
// And the one thing in the window that is not a score.
TEST(EditorSinkTest, TheBoxChoosesWhichKeyboardIsBeingRead)
{
    using antwika::music_editor::KeyLayout;
    using antwika::music_editor::kLayoutBox;

    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    ASSERT_EQ(rig.state.layout, KeyLayout::Swedish);

    clickAt(rig, centreOf(rig, kLayoutBox), 1);

    EXPECT_TRUE(rig.state.layoutOpen);

    // The second option, which is the English board.
    // Option n carries the base plus n.
    // Which is DropdownSpec's rule rather than this test's.
    const auto option = rectOf(
        rig,
        static_cast<antwika::ui::WidgetId>(
            static_cast<std::uint64_t>(
                antwika::music_editor::kLayoutOptions)
            + 1));

    clickAt(
        rig,
        antwika::gfx::Point{
            .x = option.origin.x + 1, .y = option.origin.y + 1},
        2);

    EXPECT_EQ(rig.state.layout, KeyLayout::English);
    EXPECT_FALSE(rig.state.layoutOpen);

    // And the characters follow it.
    // Shift and the full stop is a colon there and an angle here.
    press(rig, Key::Period, 3, kShift);

    EXPECT_EQ(rig.state.source, ">");
}

// Two events on one tick share that tick's folded edges.
// The next tick is what clears them.
TEST(EditorSinkTest, ReadsSeveralEventsWithinOneTick)
{
    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit1, 1);
    press(rig, Key::Digit2, 1);
    press(rig, Key::Digit3, 2);

    EXPECT_EQ(rig.state.source, "123");
}
