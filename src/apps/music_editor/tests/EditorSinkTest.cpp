#include "antwika/music_editor/EditorSink.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/MouseButton.hpp>

#include "EditorRig.hpp"

using antwika::event::TickEvent;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::music_editor::fieldFor;
using antwika::music_editor::kPanicButton;
using antwika::music_editor::kPlayButton;
using antwika::music_editor::tests::EditorRig;

namespace
{
    [[nodiscard]] TickEvent tickAt(antwika::time::Tick when)
    {
        return TickEvent{
            .tick = when,
            .event = {.name = antwika::engine::events::kTick}};
    }

    // Types one key, exactly as a recording would have held it.
    void press(EditorRig &rig, Key key, antwika::time::Tick when,
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

// The lines are re-read from what the input just did.
// Only then is the sound advanced.
TEST(EditorSinkTest, KeepsPlayingWithoutAnybodyStartingIt)
{
    EditorRig rig;

    for (antwika::time::Tick tick = 0; tick < 20; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    EXPECT_GT(rig.playback.started(), 0U);
    EXPECT_EQ(rig.playback.playedTicks(), 20U);
}

// This app defines no event of its own.
// A keystroke is recorded, and the line it lands in is regenerated.
TEST(EditorSinkTest, TypingGoesIntoTheFocusedLine)
{
    EditorRig rig;
    rig.state.lines[0].clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit5, 1);

    EXPECT_EQ(rig.state.lines[0], "5");
}

TEST(EditorSinkTest, TypingReachesTheSoundWithoutAnythingBeingReloaded)
{
    EditorRig rig;

    for (std::size_t track = 0; track < 4; ++track)
    {
        rig.state.lines[track].clear();
    }

    rig.state.cursor = 0;

    for (antwika::time::Tick tick = 0; tick < 4; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    const auto silent = rig.playback.started();

    press(rig, Key::Digit0, 5);

    for (antwika::time::Tick tick = 6; tick < 30; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    EXPECT_GT(rig.playback.started(), silent);
}

TEST(EditorSinkTest, TabMovesTheTypingToTheNextLine)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Tab, 1);

    EXPECT_EQ(rig.state.focused, 1U);
}

TEST(EditorSinkTest, ShiftTabMovesItBack)
{
    EditorRig rig;
    rig.state.focused = 2;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Tab, 1, true);

    EXPECT_EQ(rig.state.focused, 1U);
}

// Enter is the one thing a line hears that is not more music.
TEST(EditorSinkTest, EnterPausesAndResumes)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);
    EXPECT_TRUE(rig.state.paused);

    press(rig, Key::Enter, 2);
    EXPECT_FALSE(rig.state.paused);
}

TEST(EditorSinkTest, PausingStopsTheMusicalClock)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);
    ASSERT_TRUE(rig.state.paused);

    const auto played = rig.playback.playedTicks();

    for (antwika::time::Tick tick = 2; tick < 10; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

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

    for (antwika::time::Tick tick = 0; tick < 6; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    ASSERT_GT(rig.mixer.activeVoices(), 0U);

    clickAt(rig, centreOf(rig, kPanicButton), 7);

    EXPECT_EQ(rig.mixer.activeVoices(), 0U);
    EXPECT_FALSE(rig.state.paused);
}

TEST(EditorSinkTest, ClickingALineGivesItTheTyping)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, fieldFor(2)), 1);

    EXPECT_EQ(rig.state.focused, 2U);
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
    rig.state.lines[0] = "0 3";
    rig.state.cursor = 3;

    for (antwika::time::Tick tick = 0; tick < 4; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    const auto sounded = rig.playback.started();

    press(rig, Key::LeftBracket, 5);
    rig.editor.handle(tickAt(6));

    EXPECT_EQ(rig.state.lines[0], "0 3[");
    EXPECT_FALSE(rig.score.error(0).empty());

    for (antwika::time::Tick tick = 7; tick < 40; ++tick)
    {
        rig.editor.handle(tickAt(tick));
    }

    EXPECT_GT(rig.playback.started(), sounded);
}

// Only a left press reaches the layout.
// Only a pointer event says where the pointer is.
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
TEST(EditorSinkTest, RemembersThatThePointerHasBeenSomewhere)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, fieldFor(1)), 1);
    ASSERT_EQ(rig.state.focused, 1U);

    clickAt(rig, centreOf(rig, fieldFor(3)), 2);

    EXPECT_EQ(rig.state.focused, 3U);
}

// A tick carries no keys and no press.
// So nothing takes the focus and nothing is activated.
TEST(EditorSinkTest, ATickOnItsOwnActivatesNothing)
{
    EditorRig rig;
    rig.state.focused = 2;

    rig.editor.handle(tickAt(0));
    rig.editor.handle(tickAt(1));

    EXPECT_EQ(rig.state.focused, 2U);
    EXPECT_FALSE(rig.state.paused);
}
