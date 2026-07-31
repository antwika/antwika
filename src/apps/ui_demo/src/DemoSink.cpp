#include "antwika/ui_demo/DemoSink.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/ui_demo/KeyMapping.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/Widgets.hpp"

namespace antwika::ui_demo
{

    using antwika::app::locates;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::ui::kNoWidget;

    namespace
    {
        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr
                   && pressed->button == MouseButton::Left;
        }
    } // namespace

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
        // A tick's edges are cleared on the next tick's first event.
        // Clearing at the end of a tick would need this sink last.
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            refreshAndAct(false, Keyboard{});
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        located = located || locates(*decoded);
        folded.apply(*decoded);

        // The characters live here for as long as the Context does.
        // ui::Keyboard borrows them rather than owning them.
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
            }
        }

        keyboard.typed = characters;

        refreshAndAct(isLeftPress(*decoded), keyboard);
    }

    Pointer DemoSink::pointerNow(const bool pressed) const
    {
        const auto &mouse = folded.mouse();

        return Pointer{
            .position =
                located ? std::optional<Point>{antwika::app::asPoint(
                              mouse.position())}
                        : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void DemoSink::refreshAndAct(
        const bool pressed, const Keyboard &keyboard)
    {
        auto frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), keyboard, state);

        act(frame.interactions);

        // What was just typed, chosen or pressed is not in that picture.
        // So it is described once more, and the second one is drawn.
        // The same remedy ui::Context::finish() spells out.
        frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), Keyboard{}, state);

        overlay.set(std::move(frame.commands));
    }

    void DemoSink::act(const Interactions &interactions)
    {
        // A press moves the keyboard onto what it hit.
        // antwika::ui will not, until focus is already in play.
        // That is deliberate: a pointer-only caller gains no ring.
        // This showcase has a field, so it asks for one.
        state.setFocus(
            interactions.activated == kNoWidget ? interactions.focused
                                                : interactions.activated);

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
                "showing " + std::string{showcaseName(state.showcase())});
            return;
        }

        state.selectAccent(choice.index);
        state.setAccentOpen(false);
        state.setMessage(
            "accent " + std::to_string(choice.index) + " chosen");
    }

    void DemoSink::edit(const TextEdit &change)
    {
        state.setText(change.text, change.cursor);

        if (change.submitted)
        {
            state.setMessage("submitted: " + change.text);
            return;
        }

        if (change.cancelled)
        {
            // What giving up means is this application's to decide.
            // Here it is emptying the field, which is a plain answer.
            state.setText({}, 0);
            state.setMessage("cancelled");
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
                "pressed widget "
                + std::to_string(static_cast<std::uint64_t>(activated)));
        }
    }

} // namespace antwika::ui_demo
