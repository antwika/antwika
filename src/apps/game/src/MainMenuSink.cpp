#include "antwika/game/MainMenuSink.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::input::InputEvent;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

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

    MainMenuSink::MainMenuSink(
        AppModeState &mode,
        UiOverlay &overlay,
        const InputFold &input,
        const MainMenuScene &scene,
        ITickEventSink &stop)
        : mode(mode),
          overlay(overlay),
          input(input),
          scene(scene),
          stop(stop)
    {
    }

    void MainMenuSink::handle(const TickEvent &event)
    {
        // Nothing at all in any other mode: the menu is a mode of its
        // own rather than a window over one that is still running.
        if (mode.mode() != AppMode::MainMenu)
        {
            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            refreshAndAct(event, false);
            return;
        }

        // Whatever the fold was just given, since it runs first.
        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        refreshAndAct(event, isLeftPress(*decoded));
    }

    Pointer MainMenuSink::pointerNow(bool pressed) const
    {
        const auto &mouse = input.state().mouse();

        return Pointer{
            .position = input.located()
                            ? std::optional<Point>{input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void MainMenuSink::refreshAndAct(
        const TickEvent &event, bool pressed)
    {
        auto frame =
            scene.describe(overlay.canvas(), pointerNow(pressed));
        const auto activated = frame.interactions.activated;

        if (activated == menuWidgets::kNewGame)
        {
            // Staged, so the click that leaves the menu is not also read
            // as a click on the grid it reveals -- see AppMode.hpp.
            mode.request(AppMode::Playing);
        }
        else if (activated == menuWidgets::kQuit)
        {
            // The loop's own signal rather than an event on the wire:
            // the recording holds the click, and the stop follows from
            // it, exactly as a placement follows from a click on a cell.
            stop.handle(TickEvent{
                .tick = event.tick,
                .event = Event{.name = antwika::engine::events::kStop}});
        }

        overlay.set(
            std::move(frame.commands), frame.interactions.pointerOverUi);
    }

} // namespace antwika::game
