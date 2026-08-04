#include "antwika/game/MainMenuSink.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Action.hpp"

namespace antwika::game
{

    using antwika::event::Event;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
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
        ITickEventSink &stop,
        OptionsState &options,
        const OptionsScene &optionsScene,
        LocaleState &locale)
        : mode(mode),
          overlay(overlay),
          input(input),
          scene(scene),
          stop(stop),
          options(options),
          optionsScene(optionsScene),
          locale(locale)
    {
    }

    void MainMenuSink::handle(const TickEvent &event)
    {
        // Nothing at all in any other mode.
        // The menu is a mode of its own.
        // It is not a window over one that is still running.
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

        // A key press means exactly one thing on this screen.
        // The key an action is being bound to.
        // Offered before the screen is described.
        // So the row it lands on is drawn holding it in this tick.
        // Nothing is ever waiting for one anywhere else.
        // So this answers nothing at all on the menu itself.
        const auto *typed = std::get_if<KeyPressed>(&*decoded);
        if (typed != nullptr && !typed->repeat)
        {
            options.press(typed->key);
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

    void MainMenuSink::refreshOptions(bool pressed)
    {
        // Described against the language this tick is running in.
        // A change asked for below lands at the next tick's boundary.
        // So the press that asked was read on the layout it was made on.
        // Which is the whole of what LocaleState stages for.
        auto frame = optionsScene.describe(
            overlay.canvas(),
            pointerNow(pressed),
            options,
            locale.locale());
        const auto activated = frame.interactions.activated;

        if (activated == optionsWidgets::kBack)
        {
            options.setOpen(false);
        }

        for (const auto action : kActions)
        {
            if (activated == optionsWidgets::actionWidget(action))
            {
                options.await(action);
            }
        }

        for (const auto picked : antwika::i18n::kAllLocales)
        {
            if (activated == optionsWidgets::languageWidget(picked))
            {
                // Staged, never written to the wire.
                // The click that got here is already in the recording.
                // So a replay reaches this line and stages the same.
                locale.request(picked);
                options.setLocale(picked);
            }
        }

        for (const auto board : kKeyboardLayouts)
        {
            if (activated == optionsWidgets::keyboardWidget(board))
            {
                // Set outright rather than staged.
                // No layout or hit-test is a function of the board.
                // Only later typing is, and it is later either way.
                options.setKeyboard(board);
            }
        }

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

    void MainMenuSink::refreshAndAct(
        const TickEvent &event, bool pressed)
    {
        // The same screen with something else on it.
        // So the same overlay, and no mode of its own.
        if (options.open())
        {
            refreshOptions(pressed);
            return;
        }

        auto frame =
            scene.describe(overlay.canvas(), pointerNow(pressed));
        const auto activated = frame.interactions.activated;

        if (activated == menuWidgets::kOptions)
        {
            options.setOpen(true);
        }
        else if (activated == menuWidgets::kNewGame)
        {
            // Staged rather than applied here.
            // So the click that leaves the menu is not also the grid's.
            // See AppMode.hpp.
            mode.request(AppMode::CityMap);
        }
        else if (activated == menuWidgets::kWorldMap)
        {
            mode.request(AppMode::WorldMap);
        }
        else if (activated == menuWidgets::kLoadGame)
        {
            mode.request(AppMode::SaveLoad);
        }
        else if (activated == menuWidgets::kQuit)
        {
            // The loop's own signal rather than an event on the wire.
            // The recording holds the click, and the stop follows from it.
            // Exactly as a placement follows from a click on a cell.
            // The excluded line's remaining branches are allocator's.
            // The event's name is a literal too short to reach a heap.
            // The rest are the unwind edges beside that.
            stop.handle(TickEvent{ // GCOVR_EXCL_LINE
                .tick = event.tick,
                .event = Event{.name = antwika::engine::events::kStop}});
        }

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

} // namespace antwika::game
