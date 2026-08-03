#include <vector>

#include <gtest/gtest.h>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MemoryClipboard.hpp>
#include <antwika/replay/ReplaySource.hpp>

#include "antwika/music_editor/Events.hpp"
#include "antwika/music_editor/PasteSource.hpp"

using antwika::event::TickEvent;
using antwika::input::InputEventCodec;
using antwika::input::Key;
using antwika::input::KeyPressed;
using antwika::input::MemoryClipboard;
using antwika::music_editor::PasteSource;
using antwika::replay::ReplaySource;

namespace
{
    constexpr antwika::input::KeyModifiers kControl{.control = true};

    [[nodiscard]] TickEvent pressAt(
        const antwika::time::Tick tick, const KeyPressed &pressed)
    {
        const InputEventCodec codec;

        return TickEvent{.tick = tick, .event = codec.encode(pressed)};
    }
} // namespace

TEST(PasteSourceTest, TypesTheClipboardAfterAFreshControlV)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("$: drum.n(\"0\")");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[1].name, antwika::music_editor::events::kPaste);
    EXPECT_EQ(events[1].payload, "$: drum.n(\"0\")");
}

// The filter sits here, upstream of the recorder.
// So the recording holds what was typed, never the raw bytes.
TEST(PasteSourceTest, TypesTheFilteredClipboardOnly)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("a\r\nb\xE9");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[1].payload, "a\nb");
}

// A clipboard holding nothing the pane could show pastes nothing.
TEST(PasteSourceTest, AnUnpasteableClipboardPastesNothing)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("\x07\x80");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}

// The key edge itself passes through untouched and first.
// It is what a recording holds beside the paste it caused.
TEST(PasteSourceTest, LeavesTheStreamItReadUntouched)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("x");

    const auto press =
        pressAt(1, KeyPressed{.key = Key::V, .modifiers = kControl});

    ReplaySource inner({press});
    PasteSource source(inner, clipboard, codec, true);

    const auto events = source.eventsFor(1);

    ASSERT_EQ(events.size(), 2U);
    EXPECT_EQ(events[0], press.event);
}

TEST(PasteSourceTest, AnEmptyClipboardPastesNothing)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}

TEST(PasteSourceTest, ARepeatIsNotAFreshPress)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("x");

    ReplaySource inner({pressAt(
        1,
        KeyPressed{
            .key = Key::V, .modifiers = kControl, .repeat = true})});

    PasteSource source(inner, clipboard, codec, true);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}

TEST(PasteSourceTest, OnlyControlAndVAsksTheClipboardAnything)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("x");

    const InputEventCodec encoder;

    ReplaySource inner(
        {pressAt(1, KeyPressed{.key = Key::V}),
         pressAt(1, KeyPressed{.key = Key::C, .modifiers = kControl}),
         TickEvent{
             .tick = 1,
             .event = encoder.encode(
                 antwika::input::PointerMoved{
                     .position = {.x = 3, .y = 4}})},
         TickEvent{
             .tick = 1,
             .event = {.name = "music.paste", .payload = "old"}}});

    PasteSource source(inner, clipboard, codec, true);

    // The plain V, the copy, the move and the replayed paste pass.
    // None of them earns a new paste event.
    EXPECT_EQ(source.eventsFor(1).size(), 4U);
}

// A replay's pastes are in the file already.
// Reading a clipboard as well would paste twice.
// And it would paste this machine's text into somebody else's session.
TEST(PasteSourceTest, AReplayRunReadsNoClipboardAtAll)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("this machine's text");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, false);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}
