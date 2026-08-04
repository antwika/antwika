#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/KeyboardLayout.hpp"
#include "antwika/console/Typing.hpp"

namespace antwika::console::testing
{

    using antwika::event::TickEvent;
    using antwika::gfx::Point;
    using antwika::input::InputEventCodec;
    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::time::Tick;

    /**
     * @brief The first tick on which the console's field reads.
     *
     * The toggle goes down on tick 1 and each tick slides one step, so
     * the sheet stands fully open kConsoleAnimTicks ticks after that.
     * Every application that mounts the console needs this number to
     * script a session, and each one had written it out for itself.
     */
    inline constexpr Tick kOpenTick = 1 + kConsoleAnimTicks;

    /**
     * @brief Put one key press on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the press lands on.
     * @param press The edge itself, modifiers and repeat included.
     * @return The tick event a source may hand to the loop.
     */
    [[nodiscard]] inline TickEvent keyAt(
        const InputEventCodec &codec,
        const Tick tick,
        const KeyPressed press)
    {
        return TickEvent{.tick = tick, .event = codec.encode(press)};
    }

    /**
     * @brief Put one plain key press on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the press lands on.
     * @param key The key that went down.
     * @param shift Whether shift was held with it.
     * @return The tick event a source may hand to the loop.
     *
     * The short form, since shift is the only modifier a typed command
     * line ever needs; a chord or a held key goes through the overload
     * taking a whole input::KeyPressed.
     */
    [[nodiscard]] inline TickEvent keyAt(
        const InputEventCodec &codec,
        const Tick tick,
        const Key key,
        const bool shift = false)
    {
        return keyAt(
            codec,
            tick,
            KeyPressed{.key = key, .modifiers = {.shift = shift}});
    }

    /**
     * @brief Find the press that types one character on a board.
     * @param character The character to type.
     * @param layout Which board decides what a position prints.
     * @return The key and shift that print it, or a press that types
     * nothing at all when that board prints it nowhere.
     *
     * The inverse of typedCharacterFor(), searched rather than written
     * down a second time: a second table would be a second place to say
     * that the American slash position prints an underscore on a Swedish
     * board, and the two could then disagree without anything noticing.
     *
     * A character no key prints answers CapsLock, which types nothing on
     * either board -- so a script asking for one lands nothing rather
     * than landing some other character, which is what the three Swedish
     * letters the tables deliberately leave out would otherwise do.
     */
    [[nodiscard]] inline KeyPressed pressThatTypes(
        const char character,
        const KeyboardLayout layout = kDefaultKeyboardLayout)
    {
        KeyPressed press{.key = Key::CapsLock};

        for (std::size_t index = 0;
             index < antwika::input::kKeyCount;
             ++index)
        {
            const auto key = static_cast<Key>(index);

            for (const bool shift : {false, true})
            {
                if (typedCharacterFor(key, shift, layout) == character)
                {
                    press = KeyPressed{
                        .key = key, .modifiers = {.shift = shift}};
                }
            }
        }

        return press;
    }

    /**
     * @brief Append the presses that type one line, one per character.
     * @param events The script to append to.
     * @param codec What encodes an edge as an event.
     * @param tick The tick every one of the presses lands on.
     * @param text What to type.
     * @param layout Which board the run types by.
     *
     * Every character on one tick, because a command line is read and
     * executed on the tick its Enter lands on: spreading the letters
     * over ticks would only make the run longer.
     *
     * The board is a parameter rather than the library's default,
     * because which one a run types by is the application's own
     * simulation state -- see Typing.hpp -- and an application that
     * announces a different one must be able to script by it.
     */
    inline void typeText(
        std::vector<TickEvent> &events,
        const InputEventCodec &codec,
        const Tick tick,
        const std::string_view text,
        const KeyboardLayout layout = kDefaultKeyboardLayout)
    {
        for (const char character : text)
        {
            events.push_back(
                keyAt(codec, tick, pressThatTypes(character, layout)));
        }
    }

    /**
     * @brief Put a pointer button press on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the press lands on.
     * @param at Where on the canvas it landed.
     * @param button Which button went down.
     * @return The tick event a source may hand to the loop.
     */
    [[nodiscard]] inline TickEvent pressAt(
        const InputEventCodec &codec,
        const Tick tick,
        const Point at,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(antwika::input::PointerButtonPressed{
                .button = button,
                .position = {.x = at.x, .y = at.y}})};
    }

    /**
     * @brief Put a pointer button release on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the release lands on.
     * @param at Where on the canvas it landed.
     * @param button Which button came back up.
     * @return The tick event a source may hand to the loop.
     */
    [[nodiscard]] inline TickEvent releaseAt(
        const InputEventCodec &codec,
        const Tick tick,
        const Point at,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(antwika::input::PointerButtonReleased{
                .button = button,
                .position = {.x = at.x, .y = at.y}})};
    }

    /**
     * @brief Put a pointer movement on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the movement lands on.
     * @param at Where the pointer moved to.
     * @return The tick event a source may hand to the loop.
     */
    [[nodiscard]] inline TickEvent moveTo(
        const InputEventCodec &codec, const Tick tick, const Point at)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(antwika::input::PointerMoved{
                .position = {.x = at.x, .y = at.y}})};
    }

    /**
     * @brief Put a scroll notch on the wire, at a tick.
     * @param codec What encodes an edge as an event.
     * @param tick The tick the notch lands on.
     * @param vertical How many notches away from the reader.
     * @return The tick event a source may hand to the loop.
     */
    [[nodiscard]] inline TickEvent scrollAt(
        const InputEventCodec &codec,
        const Tick tick,
        const std::int32_t vertical)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(antwika::input::PointerScrolled{
                .vertical = vertical})};
    }

    /**
     * @brief Put the engine's stop on the wire, at a tick.
     * @param tick The tick to stop on.
     * @return The tick event a source may hand to the loop.
     *
     * No codec, since this is the one event in a console script that no
     * input device reported: several of these applications run until
     * something stops them, and a scripted session says when.
     */
    [[nodiscard]] inline TickEvent stopAt(const Tick tick)
    {
        // The branch left on the excluded line is the allocator's.
        // It is the heap path of a name far too short to need one.
        // engine::Engine::step() excludes the same construction.
        // So does app::TickLimitSource, for the same reason.
        // See docs/confirming-unreachable-branches.md.
        return TickEvent{ // GCOVR_EXCL_LINE
            .tick = tick,
            .event =
                antwika::event::Event{
                    .name = antwika::engine::events::kStop}};
    }

} // namespace antwika::console::testing
