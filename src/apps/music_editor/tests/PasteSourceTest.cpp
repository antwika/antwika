#include <gtest/gtest.h>

#include <vector>

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
}

TEST(PasteSourceTest, EventsFor_TypesTheClipboardOnControlV)
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

TEST(PasteSourceTest, EventsFor_TypesTheFilteredClipboardOnly)
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

TEST(PasteSourceTest, EventsFor_AnUnpasteableClipboardPastesNothing)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("\x07\x80");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}

TEST(PasteSourceTest, EventsFor_LeavesTheStreamItReadUntouched)
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

TEST(PasteSourceTest, EventsFor_AnEmptyClipboardPastesNothing)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, true);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}

TEST(PasteSourceTest, EventsFor_ARepeatIsNotAFreshPress)
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

TEST(PasteSourceTest, EventsFor_AsksTheClipboardOnlyOnControlV)
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

    EXPECT_EQ(source.eventsFor(1).size(), 4U);
}

TEST(PasteSourceTest, EventsFor_AReplayRunReadsNoClipboardAtAll)
{
    const InputEventCodec codec;
    MemoryClipboard clipboard;

    clipboard.setText("this machine's text");

    ReplaySource inner({pressAt(
        1, KeyPressed{.key = Key::V, .modifiers = kControl})});

    PasteSource source(inner, clipboard, codec, false);

    EXPECT_EQ(source.eventsFor(1).size(), 1U);
}
