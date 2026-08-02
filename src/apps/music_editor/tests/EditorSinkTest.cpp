#include "antwika/music_editor/EditorSink.hpp"

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include <antwika/event/TickEvent.hpp>
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
    // Types one key, exactly as a recording would have held it.
    void press(
        EditorRig &rig,
        Key key,
        antwika::time::Tick when,
        bool shift = false)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    KeyPressed{
                        .key = key,
                        .modifiers = {.shift = shift}})});
    }

    void clickAt(
        EditorRig &rig, antwika::gfx::Point at, antwika::time::Tick when)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = rig.codec.encode(
                    PointerButtonPressed{
                        .button = MouseButton::Left,
                        .position = {.x = at.x, .y = at.y}})});
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

// There is one thing to type into and it always has the focus.
// So a press on it changes nothing about where the typing goes.
TEST(EditorSinkTest, ClickingTheCodePaneLeavesTheTypingWhereItWas)
{
    EditorRig rig;
    rig.state.source = "0 3";
    rig.state.cursor = 3;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kCodeField), 1);

    press(rig, Key::Digit5, 2);

    EXPECT_EQ(rig.state.source, "0 35");
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

    press(rig, Key::LeftBracket, 5);
    rig.editor.handle(tickAt(6));

    EXPECT_EQ(rig.state.source, "$: bass.n(\"0 3[\")\n");
    EXPECT_TRUE(rig.score.hasError());

    tickThrough(rig, 7, 40);

    EXPECT_GT(rig.playback.started(), sounded);
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
