#include "antwika/ui_demo/DemoSink.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/ui_demo/DemoMessage.hpp"
#include "antwika/ui_demo/KeyMapping.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/Widgets.hpp"

namespace antwika::ui_demo
{

    using antwika::app::isLeftRelease;
    using antwika::app::leftPress;
    using antwika::app::locates;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerMoved;
    using antwika::ui::DragHome;
    using antwika::ui::kNoWidget;

    DemoSink::DemoSink(
        DemoState &state,
        DemoOverlay &overlay,
        const IInputEventCodec &codec,
        const DemoScene &scene)
        : state(state), overlay(overlay), codec(codec), scene(scene)
    {
    }

    void DemoSink::handle(const TickEvent &event)
    {
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            refreshAndAct(false, false, Keyboard{});
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        located = located || locates(*decoded);
        folded.apply(*decoded);

        if (isLeftRelease(*decoded))
        {
            state.setAreaDragging(DragHome::None);
        }

        std::string characters;
        Keyboard keyboard;

        if (const auto *key = std::get_if<KeyPressed>(&*decoded))
        {
            const auto meaning = uiKeyFor(key->key, key->modifiers.shift);
            if (meaning.has_value())
            {
                keyboard.keys.push_back(*meaning);
            }

            const char typed =
                typedCharacterFor(key->key, key->modifiers.shift);
            if (typed != '\0')
            {
                characters.push_back(typed);
                keyboard.keys.push_back(antwika::ui::Key::Character);
            }
        }

        keyboard.typed = characters;

        const auto *press = leftPress(*decoded);

        const bool moved =
            std::holds_alternative<PointerMoved>(*decoded);

        refreshAndAct(
            press != nullptr,
            press != nullptr
                ? press->modifiers.shift
                : moved
                      && state.areaDragging() == DragHome::Text,
            keyboard);
    }

    Pointer DemoSink::pointerNow(
        const bool pressed, const bool extends) const
    {
        const auto &mouse = folded.mouse();

        return Pointer{
            .position =
                located ? std::optional<Point>{antwika::app::asPoint(
                              mouse.position())}
                        : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed,
            .extends = extends};
    }

    void DemoSink::refreshAndAct(
        const bool pressed,
        const bool extends,
        const Keyboard &keyboard)
    {
        auto frame = scene.describe(
            overlay.canvas(), pointerNow(pressed, extends), keyboard,
            state);

        act(frame.interactions);

        if (pressed)
        {
            state.setAreaDragging(
                frame.interactions.areaPress.has_value()
                    ? frame.interactions.areaPress->home
                    : DragHome::None);
        }

        frame = scene.describe(
            overlay.canvas(), pointerNow(pressed, extends), Keyboard{},
            state);

        overlay.set(std::move(frame.commands));
    }

    void DemoSink::act(const Interactions &interactions)
    {
        state.setFocus(
            interactions.activated == kNoWidget ? interactions.focused
                                                : interactions.activated);

        if (interactions.scrolled.has_value())
        {
            state.setAreaScroll(interactions.scrolled->line);
        }

        if (interactions.chosen.has_value())
        {
            choose(*interactions.chosen);
            return;
        }

        if (interactions.edit.has_value())
        {
            edit(*interactions.edit);
            return;
        }

        press(interactions.activated);
    }

    void DemoSink::choose(const OptionChoice &choice)
    {
        if (choice.dropdown == widgets::kPicker)
        {
            state.select(choice.index);
            state.setPickerOpen(false);
            state.setMessage(
                {.id = MessageId::Showing,
                 .datum = {},
                 .argId = showcaseNameId(state.showcase())});
            return;
        }

        state.selectAccent(choice.index);
        state.setAccentOpen(false);
        state.setMessage(
            {.id = MessageId::AccentChosen,
             .datum = std::to_string(choice.index),
             .argId = std::nullopt});
    }

    void DemoSink::edit(const TextEdit &change)
    {
        if (change.field == widgets::kArea)
        {
            state.setArea(change.text, change.cursor, change.anchor);
            return;
        }

        state.setText(change.text, change.cursor);

        if (change.submitted)
        {
            state.setMessage(
                {.id = MessageId::Submitted,
                 .datum = change.text,
                 .argId = std::nullopt});
            return;
        }

        if (change.cancelled)
        {
            state.setText({}, 0);
            state.setMessage(
                {.id = MessageId::Cancelled,
                 .datum = {},
                 .argId = std::nullopt});
        }
    }

    void DemoSink::press(const WidgetId activated)
    {
        if (activated == widgets::kPicker)
        {
            state.setPickerOpen(!state.pickerOpen());
        }
        else if (activated == widgets::kPalette)
        {
            state.setAccentOpen(!state.accentOpen());
        }
        else if (activated == widgets::kCount)
        {
            state.countClick();
        }
        else if (activated == widgets::kReset)
        {
            state.resetClicks();
        }
        else if (activated != kNoWidget)
        {
            state.setMessage(
                {.id = MessageId::PressedWidget,
                 .datum = std::to_string(
                     static_cast<std::uint64_t>(activated)),
                 .argId = std::nullopt});
        }
    }

}
