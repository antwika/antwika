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

    inline constexpr Tick kOpenTick = 1 + kConsoleAnimTicks;

    [[nodiscard]] inline TickEvent keyAt(
        const InputEventCodec &codec,
        const Tick tick,
        const KeyPressed press)
    {
        return TickEvent{.tick = tick, .event = codec.encode(press)};
    }

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

    [[nodiscard]] inline KeyPressed pressThatTypes(
        const char character,
        const KeyboardLayout layout = kDefaultKeyboardLayout)
    {
        for (const bool alt : {false, true})
        {
            for (const bool shift : {false, true})
            {
                for (std::size_t index = 0;
                     index < antwika::input::kKeyCount;
                     ++index)
                {
                    const auto key = static_cast<Key>(index);

                    if (typedCharacterFor(key, shift, layout, alt)
                        == character)
                    {
                        return KeyPressed{
                            .key = key,
                            .modifiers = {.shift = shift, .alt = alt}};
                    }
                }
            }
        }

        return KeyPressed{.key = Key::CapsLock};
    }

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

    [[nodiscard]] inline TickEvent moveTo(
        const InputEventCodec &codec, const Tick tick, const Point at)
    {
        return TickEvent{
            .tick = tick,
            .event = codec.encode(antwika::input::PointerMoved{
                .position = {.x = at.x, .y = at.y}})};
    }

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

    [[nodiscard]] inline TickEvent stopAt(const Tick tick)
    {
        return TickEvent{ // GCOVR_EXCL_LINE
            .tick = tick,
            .event =
                antwika::event::Event{
                    .name = antwika::engine::events::kStop}};
    }

}
