#include "antwika/game/MenuSink.hpp"

#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::ui::kNoWidget;

    namespace
    {
        [[nodiscard]] bool isMenuKeyPress(
            const InputEvent &event) noexcept
        {
            const auto *pressed = std::get_if<KeyPressed>(&event);

            return pressed != nullptr && pressed->key == kMenuKey;
        }

        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr
                   && pressed->button == MouseButton::Left;
        }
    } // namespace

    MenuSink::MenuSink(
        MenuState &state,
        UiOverlay &overlay,
        const InputFold &input,
        const MainMenu &menu,
        std::optional<std::reference_wrapper<ITickEventSink>> whenClosed)
        : state(state),
          overlay(overlay),
          input(input),
          menu(menu),
          whenClosed(whenClosed)
    {
    }

    void MenuSink::handle(const TickEvent &event)
    {
        // What was activated belongs to the tick it happened in.
        // It is cleared on the next tick's first event.
        // InputFold clears its edges there too, for the same reason.
        // Nothing inside a tick can then read one that is gone.
        if (handledTick != event.tick)
        {
            state.activated.reset();
            handledTick = event.tick;
        }

        const bool wasOpen = state.open;

        if (event.event.name == antwika::engine::events::kTick)
        {
            if (state.open)
            {
                // Described again for the renderer about to paint.
                // What it paints shows the state this tick ends with.
                refreshAndAct(false);
            }
        }
        else if (input.current().has_value())
        {
            react(*input.current());
        }

        // A modal takes what it covers, and it covers everything.
        // So nothing behind it sees an event while it is up.
        // That includes the press that raised it and the one that ended it.
        if (!wasOpen && !state.open && whenClosed.has_value())
        {
            whenClosed->get().handle(event);
        }
    }

    void MenuSink::react(const InputEvent &decoded)
    {
        if (isMenuKeyPress(decoded))
        {
            state.open = !state.open;

            if (state.open)
            {
                refreshAndAct(false);

                return;
            }

            // Nothing of the menu is left to paint.
            // And nothing of it is left covering the grid.
            overlay.set({}, false);

            return;
        }

        if (state.open)
        {
            refreshAndAct(isLeftPress(decoded));
        }
    }

    Pointer MenuSink::pointerNow(bool pressed) const
    {
        const auto &mouse = input.state().mouse();

        return Pointer{
            .position = input.located()
                            ? std::optional<Point>{input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void MenuSink::refreshAndAct(bool pressed)
    {
        auto frame =
            menu.describe(overlay.canvas(), pointerNow(pressed), state);
        const auto activated = frame.interactions.activated;

        act(activated);

        if (!state.open)
        {
            overlay.set({}, false);

            return;
        }

        // The language or the entry list has just changed.
        // The picture above predates that, so it is described again.
        // See Context::finish() on why describing twice is the remedy.
        if (activated != kNoWidget)
        {
            frame = menu.describe(
                overlay.canvas(), pointerNow(pressed), state);
        }

        overlay.set(
            std::move(frame.commands), frame.interactions.pointerOverUi);
    }

    void MenuSink::act(WidgetId activated)
    {
        if (const auto language = languageFor(activated);
            language.has_value())
        {
            state.language = *language;

            return;
        }

        const auto entry = entryFor(activated);
        if (!entry.has_value())
        {
            return;
        }

        state.activated = entry;

        if (*entry == MenuEntry::PlayGame)
        {
            state.gameBegun = true;
        }

        state.open = !leavesMenu(*entry);
    }

} // namespace antwika::game
