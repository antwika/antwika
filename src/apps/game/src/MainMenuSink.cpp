#include "antwika/game/MainMenuSink.hpp"

#include <optional>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
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

    using antwika::app::isLeftPress;

    namespace
    {
    }

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
        if (mode.mode() != AppMode::MainMenu)
        {
            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            refreshAndAct(event, false);
            return;
        }

        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

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
                locale.request(picked);
                options.setLocale(picked);
            }
        }

        for (const auto board : kKeyboardLayouts)
        {
            if (activated == optionsWidgets::keyboardWidget(board))
            {
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
            stop.handle(TickEvent{ // GCOVR_EXCL_LINE
                .tick = event.tick,
                .event = Event{.name = antwika::engine::events::kStop}});
        }

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            frame.interactions.pointerOverUi);
    }

}
