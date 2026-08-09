#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>
#include <unistd.h>

#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/KeyModifiers.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/Events.hpp"
#include "antwika/music_editor/ScoreFiles.hpp"
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

    void paste(
        EditorRig &rig,
        const std::string &text,
        antwika::time::Tick when)
    {
        rig.editor.handle(
            TickEvent{
                .tick = when,
                .event = {
                    .name = antwika::music_editor::events::kPaste,
                    .payload = text}});
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

        return antwika::gfx::Point{
            .x = rect.origin.x
                + static_cast<std::int32_t>(rect.size.width) / 2,
            .y = rect.origin.y
                + static_cast<std::int32_t>(rect.size.height) / 2};
    }

    [[nodiscard]] antwika::gfx::Point modalCentreOf(
        EditorRig &rig, antwika::ui::WidgetId id)
    {
        const auto frame = rig.scene.describeModal(
            rig.state, antwika::music_editor::tests::kCanvas, {}, {});

        const auto found = frame.rects.find(id);
        EXPECT_TRUE(found.has_value());

        const auto rect = found.value_or(antwika::gfx::Rect{});

        return antwika::gfx::Point{
            .x = rect.origin.x
                + static_cast<std::int32_t>(rect.size.width) / 2,
            .y = rect.origin.y
                + static_cast<std::int32_t>(rect.size.height) / 2};
    }

    [[nodiscard]] antwika::ui::WidgetId menuOption(std::size_t at)
    {
        return antwika::ui::WidgetId{
            static_cast<std::uint64_t>(
                antwika::music_editor::kMenuOptions)
            + at};
    }

    [[nodiscard]] antwika::ui::WidgetId speedOption(std::size_t at)
    {
        return antwika::ui::WidgetId{
            static_cast<std::uint64_t>(
                antwika::music_editor::kSpeedOptions)
            + at};
    }

    [[nodiscard]] std::string freshDirectory(const std::string &name)
    {
        const auto path =
            std::filesystem::temp_directory_path()
            / ("antwika-editor." + std::to_string(::getpid()))
            / name;

        std::filesystem::remove_all(path);

        return path.string();
    }
}

TEST(EditorSinkTest, Handle_DrawsSomethingOnTheVeryFirstTick)
{
    EditorRig rig;

    rig.editor.handle(tickAt(0));

    EXPECT_FALSE(rig.editor.commands().empty());
}

TEST(EditorSinkTest, Handle_KeepsPlayingWithoutBeingStarted)
{
    EditorRig rig;

    tickThrough(rig, 0, 20);

    EXPECT_GT(rig.playback.started(), 0U);
    EXPECT_EQ(rig.playback.playedTicks(), 20U);
}

TEST(EditorSinkTest, Handle_TypingGoesIntoTheDocumentAtTheCaret)
{
    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit5, 1);

    EXPECT_EQ(rig.state.source, "5");
    EXPECT_EQ(rig.state.cursor, 1U);
}

TEST(EditorSinkTest, Handle_LeavesTheRestWhenTypingInside)
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

TEST(EditorSinkTest, Handle_TabIndentsByTwoRatherThanMovingAnything)
{
    EditorRig rig;
    rig.state.source.clear();
    rig.state.cursor = 0;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Tab, 1);

    EXPECT_EQ(rig.state.source, "  ");
    EXPECT_EQ(rig.state.cursor, 2U);
}

TEST(EditorSinkTest, Handle_BackspacesBeforeTheCaret)
{
    EditorRig rig;
    rig.state.source = "0 3";
    rig.state.cursor = 3;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Backspace, 1);

    EXPECT_EQ(rig.state.source, "0 ");
}

TEST(EditorSinkTest, Handle_EnterIsANewLineRatherThanAPause)
{
    EditorRig rig;
    rig.state.source = "$: bass.n(\"0\")";
    rig.state.cursor = 14;

    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);

    EXPECT_EQ(rig.state.source, "$: bass.n(\"0\")\n");
    EXPECT_FALSE(rig.state.paused);
}

TEST(EditorSinkTest, Handle_TheArrowsWalkTheCaretBetweenLines)
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

TEST(EditorSinkTest, Handle_ReachesTheSoundWithoutAReload)
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

TEST(EditorSinkTest, Handle_EscapePausesAndResumes)
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

TEST(EditorSinkTest, Handle_PausingStopsTheMusicalClock)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Escape, 1);
    ASSERT_TRUE(rig.state.paused);

    const auto played = rig.playback.playedTicks();

    tickThrough(rig, 2, 10);

    EXPECT_EQ(rig.playback.playedTicks(), played);
}

TEST(EditorSinkTest, Handle_TheButtonPausesToo)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPlayButton), 1);

    EXPECT_TRUE(rig.state.paused);
}

TEST(EditorSinkTest, Handle_SilencesEveryVoiceOnTheButton)
{
    EditorRig rig;
    rig.state.source = "$: bell.n(\"0*8\")\n";

    tickThrough(rig, 0, 6);

    ASSERT_GT(rig.mixer.activeVoices(), 0U);

    clickAt(rig, centreOf(rig, kPanicButton), 7);

    EXPECT_EQ(rig.mixer.activeVoices(), 0U);
    EXPECT_FALSE(rig.state.paused);
}

TEST(EditorSinkTest, Handle_PutsTheCaretAtTheEndBelowText)
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

TEST(EditorSinkTest, Handle_ClickingACharacterPutsTheCaretOnIt)
{
    EditorRig rig;
    rig.state.source = "abcd\nefgh";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 1, 2), 1);

    EXPECT_EQ(rig.state.cursor, 7U);
    EXPECT_EQ(rig.state.anchor, 7U);

    press(rig, Key::Digit5, 2);

    EXPECT_EQ(rig.state.source, "abcd\nef5gh");
}

TEST(EditorSinkTest, Handle_CarriesASelectionOnAShiftClick)
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

TEST(EditorSinkTest, Handle_SelectsWhatADragCrossed)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 0), 1);
    moveTo(rig, cellOf(rig, 0, 3), 1);

    EXPECT_EQ(rig.state.cursor, 3U);
    EXPECT_EQ(rig.state.anchor, 0U);

    releaseAt(rig, cellOf(rig, 0, 3), 1);
    moveTo(rig, cellOf(rig, 0, 1), 2);

    EXPECT_EQ(rig.state.cursor, 3U);
}

TEST(EditorSinkTest, Handle_LeavesTheCaretWhenTypingMidDrag)
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

TEST(EditorSinkTest, Handle_SelectsNothingOnAPresslessMove)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPanicButton), 1);
    moveTo(rig, cellOf(rig, 0, 3), 2);

    EXPECT_EQ(rig.state.cursor, 0U);
}

TEST(EditorSinkTest, Handle_AnEventItCannotDecodeChangesNothing)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto before = rig.state;

    rig.editor.handle(
        TickEvent{.tick = 1, .event = {.name = "something.else"}});

    EXPECT_EQ(rig.state, before);
}

TEST(EditorSinkTest, Handle_ReportsARefusedLineAndKeepsSound)
{
    EditorRig rig;
    rig.state.source = "$: bass.n(\"0 3\")\n";
    rig.state.cursor = 14;

    tickThrough(rig, 0, 4);

    const auto sounded = rig.playback.started();

    press(rig, Key::Digit8, 5, kAlt);
    rig.editor.handle(tickAt(6));

    EXPECT_EQ(rig.state.source, "$: bass.n(\"0 3[\")\n");
    EXPECT_TRUE(rig.score.hasError());

    tickThrough(rig, 7, 40);

    EXPECT_GT(rig.playback.started(), sounded);
}

TEST(EditorSinkTest, Handle_ARightReleaseEndsNoDrag)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    clickAt(rig, cellOf(rig, 0, 0), 1);
    ASSERT_EQ(rig.state.dragging, antwika::ui::DragHome::Text);

    rig.editor.handle(
        TickEvent{
            .tick = 2,
            .event = rig.codec.encode(
                antwika::input::PointerButtonReleased{
                    .button = MouseButton::Right,
                    .position = {.x = 0, .y = 0}})});

    EXPECT_EQ(rig.state.dragging, antwika::ui::DragHome::Text);
}

TEST(EditorSinkTest, Handle_ARightPressPressesNothing)
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

TEST(EditorSinkTest, Handle_RemembersThePointerHasMoved)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kPanicButton), 1);

    clickAt(rig, centreOf(rig, kPlayButton), 2);

    EXPECT_TRUE(rig.state.paused);
}

TEST(EditorSinkTest, Handle_ATickOnItsOwnActivatesNothing)
{
    EditorRig rig;

    rig.editor.handle(tickAt(0));
    const auto before = rig.state;

    rig.editor.handle(tickAt(1));

    EXPECT_EQ(rig.state, before);
}

TEST(EditorSinkTest, Handle_AKeyThatSaysNothingChangesNothing)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    const auto before = rig.state;

    press(rig, Key::F1, 1);

    EXPECT_EQ(rig.state, before);
}

TEST(EditorSinkTest, Handle_SelectsWithoutMovingTheFarEnd)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 1;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);

    EXPECT_EQ(rig.state.cursor, 3U);
    EXPECT_EQ(rig.state.anchor, 1U);

    press(rig, Key::Digit5, 3);

    EXPECT_EQ(rig.state.source, "a5d");
}

TEST(EditorSinkTest, Handle_CopiesOutAndPastesWhatItCarries)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);
    press(rig, Key::C, 3, kControl);

    EXPECT_EQ(rig.state.clipboard, "ab");
    EXPECT_EQ(rig.osClipboard.text(), "ab");
    EXPECT_EQ(rig.state.source, "abcd");

    press(rig, Key::ArrowRight, 4);
    press(rig, Key::ArrowRight, 5);
    press(rig, Key::ArrowRight, 6);
    paste(rig, rig.osClipboard.text(), 7);

    EXPECT_EQ(rig.state.source, "abcdab");
    EXPECT_EQ(rig.state.cursor, 6U);
}

TEST(EditorSinkTest, Handle_ControlAndVAloneTypesNothing)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    press(rig, Key::V, 1, kControl);

    EXPECT_EQ(rig.state.source, "abcd");
}

TEST(EditorSinkTest, Handle_PastesOverAWholeSelection)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 1;
    rig.editor.handle(tickAt(0));

    press(rig, Key::ArrowRight, 1, kShift);
    press(rig, Key::ArrowRight, 2, kShift);
    paste(rig, "XY", 3);

    EXPECT_EQ(rig.state.source, "aXYd");
    EXPECT_EQ(rig.state.cursor, 3U);
}

TEST(EditorSinkTest, Handle_CopiesNowhereWithoutAClipboard)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;

    antwika::music_editor::EditorSink bare{
        rig.state,
        rig.score,
        rig.playback,
        rig.codec,
        rig.scene,
        antwika::music_editor::tests::kCanvas,
        antwika::music_editor::WaveRenderDesc{
            .rate = rig.format.rate,
            .framesPerCycle =
                antwika::sequencer::Rational(rig.format.rate)},
        std::nullopt,
        rig.stopSignal,
        rig.scoresDirectory,
        true};

    bare.handle(tickAt(0));

    bare.handle(
        TickEvent{
            .tick = 1,
            .event = rig.codec.encode(
                KeyPressed{.key = Key::ArrowRight, .modifiers = kShift})});
    bare.handle(
        TickEvent{
            .tick = 2,
            .event = rig.codec.encode(
                KeyPressed{.key = Key::C, .modifiers = kControl})});

    EXPECT_EQ(rig.state.clipboard, "a");
    EXPECT_EQ(rig.osClipboard.text(), "");
}

TEST(EditorSinkTest, Handle_CuttingTakesTheSelectionWithIt)
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

    paste(rig, rig.osClipboard.text(), 4);
    paste(rig, rig.osClipboard.text(), 5);

    EXPECT_EQ(rig.state.source, "abcdcd");
}

TEST(EditorSinkTest, Handle_KeepsTheLastCopyWhenCopyingNothing)
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

TEST(EditorSinkTest, Handle_DeletesTheCharacterBeforeTheCaret)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 1;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Delete, 1);

    EXPECT_EQ(rig.state.source, "acd");
}

TEST(EditorSinkTest, Handle_ScrollsTheWheelAndStopsAtTheEnds)
{
    EditorRig rig;
    rig.state.source = std::string(200, '\n');
    rig.state.cursor = 0;
    rig.editor.handle(tickAt(0));

    wheel(rig, -1, 1);

    EXPECT_EQ(rig.state.scroll, 3U);

    wheel(rig, 1, 2);

    EXPECT_EQ(rig.state.scroll, 0U);

    wheel(rig, 4, 3);

    EXPECT_EQ(rig.state.scroll, 0U);

    wheel(rig, -300, 4);

    EXPECT_LT(rig.state.scroll, 200U);
    EXPECT_GT(rig.state.scroll, 0U);
}

TEST(EditorSinkTest, Handle_ScrollsTypingBelowThePaneIntoView)
{
    EditorRig rig;
    rig.state.source = std::string(200, '\n');
    rig.state.cursor = rig.state.source.size();
    rig.editor.handle(tickAt(0));

    press(rig, Key::Digit5, 1);

    EXPECT_GT(rig.state.scroll, 100U);
}

TEST(EditorSinkTest, Handle_ChoosesTheClockSpeedInTheBox)
{
    using antwika::music_editor::kSpeedBox;

    EditorRig rig;
    rig.editor.handle(tickAt(0));

    ASSERT_EQ(rig.state.speed, antwika::music_editor::kNormalSpeed);

    clickAt(rig, centreOf(rig, kSpeedBox), 1);

    EXPECT_TRUE(rig.state.speedOpen);

    const auto option = rectOf(rig, speedOption(3));

    clickAt(
        rig,
        antwika::gfx::Point{
            .x = option.origin.x + 1, .y = option.origin.y + 1},
        2);

    EXPECT_EQ(rig.state.speed, 3U);
    EXPECT_FALSE(rig.state.speedOpen);
}

TEST(EditorSinkTest, Handle_ClosesTheBoxOnTheSameSpeed)
{
    using antwika::music_editor::kNormalSpeed;
    using antwika::music_editor::kSpeedBox;

    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, kSpeedBox), 1);

    const auto option = rectOf(rig, speedOption(kNormalSpeed));

    clickAt(
        rig,
        antwika::gfx::Point{
            .x = option.origin.x + 1, .y = option.origin.y + 1},
        2);

    EXPECT_EQ(rig.state.speed, kNormalSpeed);
    EXPECT_FALSE(rig.state.speedOpen);
}

TEST(EditorSinkTest, Handle_ChoosesTheKeyboardInTheBox)
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

    press(rig, Key::Period, 3, kShift);

    EXPECT_EQ(rig.state.source, ">");
}

TEST(EditorSinkTest, Handle_ReadsSeveralEventsWithinOneTick)
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

TEST(EditorSinkTest, Handle_OpensTheMenuAndQuitsTheLoop)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, antwika::music_editor::kMenuBox), 1);

    EXPECT_TRUE(rig.state.menuOpen);
    EXPECT_FALSE(rig.stopSignal.stopped());

    clickAt(rig, centreOf(rig, menuOption(3)), 2);

    EXPECT_FALSE(rig.state.menuOpen);
    EXPECT_TRUE(rig.stopSignal.stopped());
}

TEST(EditorSinkTest, Handle_NewEmptiesThePage)
{
    EditorRig rig;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, antwika::music_editor::kMenuBox), 1);
    clickAt(rig, centreOf(rig, menuOption(0)), 2);

    EXPECT_EQ(rig.state.source, "");
    EXPECT_EQ(rig.state.cursor, 0U);
    EXPECT_EQ(rig.state.scroll, 0U);
    EXPECT_EQ(
        rig.state.modal, antwika::music_editor::Modal::None);
}

TEST(EditorSinkTest, Handle_WritesTheScoreAndListsIt)
{
    EditorRig rig(freshDirectory("saving"));
    rig.state.source = "$: drum.n(\"0\")\n";
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, antwika::music_editor::kMenuBox), 1);
    clickAt(rig, centreOf(rig, menuOption(1)), 2);

    ASSERT_EQ(rig.state.modal, antwika::music_editor::Modal::Save);

    press(rig, Key::A, 3);
    press(rig, Key::B, 4);

    EXPECT_EQ(rig.state.fileName, "ab");

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::kSaveConfirm),
        5);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
    ASSERT_EQ(rig.state.scores.size(), 1U);
    EXPECT_EQ(rig.state.scores[0], "ab");

    EXPECT_EQ(
        antwika::music_editor::loadScore(
            antwika::music_editor::scorePath(
                rig.scoresDirectory, "ab")),
        "$: drum.n(\"0\")\n");
}

TEST(EditorSinkTest, Handle_EnterInTheNameFieldSaves)
{
    EditorRig rig(freshDirectory("submitting"));
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.fileName = "beat";
    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
    ASSERT_EQ(rig.state.scores.size(), 1U);
    EXPECT_EQ(rig.state.scores[0], "beat");
}

TEST(EditorSinkTest, Handle_ReplaysASaveWithoutTouchingDisk)
{
    const auto directory = freshDirectory("replaying");
    EditorRig rig(directory, false);
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.fileName = "beat";
    rig.editor.handle(tickAt(0));

    press(rig, Key::Enter, 1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
    ASSERT_EQ(rig.state.scores.size(), 1U);
    EXPECT_EQ(rig.state.scores[0], "beat");
    EXPECT_FALSE(std::filesystem::exists(
        antwika::music_editor::scorePath(directory, "beat")));
}

TEST(EditorSinkTest, Handle_ANameOfNothingIsRefusedWithANotice)
{
    EditorRig rig(freshDirectory("nameless"));
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.fileName = "..//";
    rig.editor.handle(tickAt(0));

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::kSaveConfirm),
        1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::Save);
    EXPECT_EQ(rig.state.notice, "name it first");
    EXPECT_TRUE(rig.state.scores.empty());
}

TEST(EditorSinkTest, Handle_ShowsANoticeWhenTheDiskRefuses)
{
    const auto blocked = freshDirectory("blocked");

    std::filesystem::create_directories(
        std::filesystem::path(blocked).parent_path());
    std::ofstream(blocked) << "in the way";

    EditorRig rig(blocked);
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.fileName = "beat";
    rig.editor.handle(tickAt(0));

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::kSaveConfirm),
        1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::Save);
    EXPECT_FALSE(rig.state.notice.empty());
    EXPECT_TRUE(rig.state.scores.empty());
}

TEST(EditorSinkTest, Handle_LoadingReplacesThePage)
{
    const auto directory = freshDirectory("loading");

    antwika::music_editor::saveScore(
        antwika::music_editor::scorePath(directory, "beat"),
        "$: bell.n(\"7\")\n");

    EditorRig rig(directory);
    rig.state.scores = {"beat"};
    rig.state.cursor = 3;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, antwika::music_editor::kMenuBox), 1);
    clickAt(rig, centreOf(rig, menuOption(2)), 2);

    ASSERT_EQ(rig.state.modal, antwika::music_editor::Modal::Load);

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::loadOption(0)),
        3);

    EXPECT_EQ(rig.state.source, "$: bell.n(\"7\")\n");
    EXPECT_EQ(rig.state.cursor, 0U);
    EXPECT_EQ(rig.state.scroll, 0U);
    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
}

TEST(EditorSinkTest, Handle_ShowsANoticeOnAnUnreadableScore)
{
    EditorRig rig(freshDirectory("ghostly"));
    rig.state.scores = {"ghost"};
    rig.state.modal = antwika::music_editor::Modal::Load;
    rig.editor.handle(tickAt(0));

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::loadOption(0)),
        1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::Load);
    EXPECT_FALSE(rig.state.notice.empty());
}

TEST(EditorSinkTest, Handle_CancelsABoxAndKeepsThePage)
{
    EditorRig rig;
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.notice = "stale";
    rig.editor.handle(tickAt(0));

    const auto before = rig.state.source;

    clickAt(
        rig,
        modalCentreOf(rig, antwika::music_editor::kModalCancel),
        1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
    EXPECT_EQ(rig.state.notice, "");
    EXPECT_EQ(rig.state.source, before);
}

TEST(EditorSinkTest, Handle_ClosesABoxOnEscapeRatherThanPause)
{
    EditorRig rig;
    rig.state.modal = antwika::music_editor::Modal::Load;
    rig.editor.handle(tickAt(0));

    press(rig, Key::Escape, 1);

    EXPECT_EQ(rig.state.modal, antwika::music_editor::Modal::None);
    EXPECT_FALSE(rig.state.paused);

    press(rig, Key::Escape, 2);

    EXPECT_TRUE(rig.state.paused);
}

TEST(EditorSinkTest, Handle_SendsTypingToAnOpenBoxAlone)
{
    EditorRig rig;
    rig.state.source = "abcd";
    rig.state.cursor = 0;
    rig.state.modal = antwika::music_editor::Modal::Save;
    rig.state.fileName.clear();
    rig.state.fileCursor = 0;
    rig.editor.handle(tickAt(0));

    press(rig, Key::A, 1);

    EXPECT_EQ(rig.state.fileName, "a");
    EXPECT_EQ(rig.state.source, "abcd");
    EXPECT_EQ(rig.state.cursor, 0U);
}

TEST(EditorSinkTest, Handle_MovesNoCaretOnChoosingAnOption)
{
    EditorRig rig;
    rig.state.cursor = 3;
    rig.editor.handle(tickAt(0));

    clickAt(rig, centreOf(rig, antwika::music_editor::kLayoutBox), 1);

    const auto option = antwika::ui::WidgetId{
        static_cast<std::uint64_t>(
            antwika::music_editor::kLayoutOptions)
        + 1};

    clickAt(rig, centreOf(rig, option), 2);

    EXPECT_EQ(
        rig.state.layout, antwika::music_editor::KeyLayout::English);
    EXPECT_EQ(rig.state.cursor, 3U);
}

TEST(EditorSinkTest, Handle_LightsTheSoundingNotes)
{
    EditorRig rig;
    rig.state.source = "$: bell.n(\"0\")\n";

    tickThrough(rig, 0, 3);

    const auto ground =
        antwika::music_editor::editorTheme().highlight;

    bool lit = false;

    for (const auto &command : rig.editor.commands())
    {
        const auto *fill =
            std::get_if<antwika::ui::FillRect>(&command);

        if (fill != nullptr && fill->color == ground)
        {
            lit = true;
        }
    }

    EXPECT_TRUE(lit);
}

TEST(EditorSinkTest, Handle_DrawsAWaveformLinesAudio)
{
    EditorRig rig;
    rig.state.source =
        "$: n(\"0\").s(square).base(1).gain(.5)"
        ".att(0).dec(0).sus(1).hold(2000).rel(50).waveform()\n";

    tickThrough(rig, 0, 2);

    constexpr antwika::gfx::Color kWaveInk{
        .red = 110, .green = 170, .blue = 235, .alpha = 255};

    std::size_t columns = 0;

    for (const auto &command : rig.editor.commands())
    {
        const auto *fill =
            std::get_if<antwika::ui::FillRect>(&command);

        if (fill != nullptr && fill->color == kWaveInk)
        {
            ++columns;
        }
    }

    EXPECT_GT(columns, 500U);
}
