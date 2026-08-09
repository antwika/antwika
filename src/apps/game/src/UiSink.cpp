#include "antwika/game/UiSink.hpp"

#include <cstddef>
#include <optional>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/MenuItem.hpp"

namespace antwika::game
{

    using antwika::input::InputEvent;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::ui::kNoWidget;
    using antwika::ui::WidgetId;

    using antwika::app::isLeftPress;

    namespace
    {
    }

    UiSink::UiSink(
        const Camera &camera,
        UiOverlay &overlay,
        const InputFold &input,
        const Toolbar &toolbar,
        const PauseState &pause,
        MapViewState &view,
        IMenuCommands &commands,
        RoadDrag &drag,
        const MenuModalScene &modal,
        ViewCommands &viewCommands,
        const CityRatings &ratings,
        const GameState &state)
        : camera(camera),
          overlay(overlay),
          input(input),
          toolbar(toolbar),
          pause(pause),
          view(view),
          commands(commands),
          drag(drag),
          modal(modal),
          viewCommands(viewCommands),
          ratings(ratings),
          state(state)
    {
    }

    void UiSink::handle(const TickEvent &event)
    {
        tick = event.tick;

        if (event.event.name == antwika::engine::events::kTick)
        {
            refreshAndAct(false);
            return;
        }

        const auto &decoded = input.current();
        if (!decoded.has_value())
        {
            return;
        }

        refreshAndAct(isLeftPress(*decoded));
    }

    bool UiSink::menuOpen() const noexcept
    {
        return modalOpen;
    }

    bool UiSink::gameMenuOpen() const noexcept
    {
        return listOpen;
    }

    bool UiSink::viewMenuOpen() const noexcept
    {
        return viewOpen;
    }

    void UiSink::openModal()
    {
        modalOpen = true;

        drag.finish();
    }

    Pointer UiSink::pointerNow(bool pressed) const
    {
        const auto &mouse = input.state().mouse();

        return Pointer{
            .position = input.located()
                            ? std::optional<Point>{input.pointer()}
                            : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void UiSink::refreshAndAct(bool pressed)
    {
        auto frame = describeNow(pressed);

        bool dismissed = false;

        if (!modalOpen)
        {
            dismissed = actOnUi(frame.interactions, pressed);

            if (dismissed || frame.interactions.activated != kNoWidget)
            {
                frame = describeNow(pressed);
            }
        }

        bool covered = frame.interactions.pointerOverUi || dismissed;

        if (modalOpen)
        {
            auto over = modal.describe(overlay.canvas(), pointerNow(pressed));

            covered = over.interactions.pointerOverUi;

            const auto chosen = over.interactions.activated;

            frame.commands.insert(
                frame.commands.end(),
                over.commands.begin(),
                over.commands.end());

            actOnModal(chosen);
        }

        overlay.set(
            std::move(frame.commands),
            std::move(frame.hoverTargets),
            covered);
    }

    bool UiSink::actOnUi(const Interactions &interactions, bool pressed)
    {
        const auto activated = interactions.activated;

        if (listOpen && pressed)
        {
            if (interactions.chosen.has_value())
            {
                chooseFrom(interactions.chosen->index);

                return false;
            }

            if (activated != widgets::kGameMenu)
            {
                listOpen = false;

                return true;
            }
        }

        if (viewOpen && pressed)
        {
            if (interactions.chosen.has_value())
            {
                chooseView(interactions.chosen->index);

                return false;
            }

            if (activated != widgets::kViewMenu)
            {
                viewOpen = false;

                return true;
            }
        }

        actOnBar(activated);

        return false;
    }

    void UiSink::chooseFrom(std::size_t index)
    {
        listOpen = false;

        const auto item = static_cast<MenuItem>(index);

        if (item == MenuItem::NewGame)
        {
            commands.newGame();
        }
        else if (item == MenuItem::SaveGame || item == MenuItem::LoadGame)
        {
            commands.openSaves();
        }
        else if (item == MenuItem::MainMenu)
        {
            commands.mainMenu();
        }
        else
        {
            commands.worldMap();
        }
    }

    void UiSink::chooseView(std::size_t index)
    {
        viewOpen = false;

        view.set(static_cast<MapView>(index));
    }

    void UiSink::actOnBar(WidgetId activated)
    {
        if (activated == widgets::kZoomIn)
        {
            viewCommands.zoomIn();
        }
        else if (activated == widgets::kZoomOut)
        {
            viewCommands.zoomOut();
        }
        else if (activated == widgets::kResetView)
        {
            viewCommands.resetView();
        }
        else if (activated == widgets::kPauseResume)
        {
            viewCommands.togglePause();
        }
        else if (activated == widgets::kMenu)
        {
            openModal();
        }
        else if (activated == widgets::kGameMenu)
        {
            viewOpen = false;
            listOpen = !listOpen;
        }
        else if (activated == widgets::kViewMenu)
        {
            listOpen = false;
            viewOpen = !viewOpen;
        }
        else
        {
            selectFrom(activated);
        }
    }

    void UiSink::actOnModal(WidgetId activated)
    {
        if (activated == modalWidgets::kMainMenu)
        {
            commands.mainMenu();

            modalOpen = false;
        }
        else if (activated == modalWidgets::kResume)
        {
            modalOpen = false;
        }
    }

    Frame UiSink::describeNow(bool pressed) const
    {
        return toolbar.describe(
            overlay.canvas(),
            pointerNow(pressed),
            camera,
            overlay.tool(),
            pause.paused(),
            tick,
            ratings,
            listOpen,
            view.view(),
            viewOpen,
            state.money);
    }

    void UiSink::selectFrom(WidgetId activated)
    {
        for (std::size_t index = 0; index < kBuildToolCount; ++index)
        {
            const auto tool = static_cast<BuildTool>(index);

            if (activated == widgets::toolWidget(tool))
            {
                overlay.select(tool);
                return;
            }
        }
    }

}
