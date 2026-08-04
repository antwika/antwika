#include "antwika/game/UiSink.hpp"

#include <cstddef>
#include <optional>
#include <variant>

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
        // Off the event rather than counted here.
        // So the number on the bar is the simulation's own.
        tick = event.tick;

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            // What it paints then shows the state this tick ends with.
            refreshAndAct(false);
            return;
        }

        // Whatever the fold was just given, since it runs first.
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
        // No guard on it being open already, deliberately.
        // Both calls below say what they want rather than flip it.
        // So asking twice is asking once, with no branch to be.
        modalOpen = true;

        // Whatever route was being dragged out is over, and lays none.
        // What a drag lays is what its release said.
        // A release arriving over the modal never said it.
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
        // The bar is described whether or not the modal is up.
        // The modal is drawn over the city rather than instead of it.
        auto frame = describeNow(pressed);

        // A press that did nothing but put the game menu away.
        // It landed on no widget, and it is still the UI's press.
        bool dismissed = false;

        // While the modal is up the bar is a picture and nothing else.
        // A press is resolved against the modal alone, never through it.
        if (!modalOpen)
        {
            dismissed = actOnUi(frame.interactions, pressed);

            // The zoom the bar reports has just changed.
            // So has the pause button's label, and the menu's list.
            // So it is described once more.
            // Otherwise it would show the level it was pressed at.
            if (dismissed || frame.interactions.activated != kNoWidget)
            {
                frame = describeNow(pressed);
            }
        }

        // The dismissed press is over nothing the second picture drew.
        // Reporting it covered anyway is what keeps it off the city.
        bool covered = frame.interactions.pointerOverUi || dismissed;

        // Read again, since the bar's menu button may have just set it.
        if (modalOpen)
        {
            auto over = modal.describe(overlay.canvas(), pointerNow(pressed));

            // The scrim covers the canvas, so this is the whole answer.
            // Which is what keeps the press off the city underneath.
            covered = over.interactions.pointerOverUi;

            const auto chosen = over.interactions.activated;

            // Appended after the bar's, so paint order puts it in front.
            // Which is how "on top" is said where gfx has no depth.
            frame.commands.insert(
                frame.commands.end(),
                over.commands.begin(),
                over.commands.end());

            // Acted on after the picture is settled.
            // So a modal closed by this press is what that press saw.
            // The engine.tick ending this tick describes it away.
            // Ahead of anything painting -- see UiSink.hpp.
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

        // Only a press may put a list away.
        // Nothing else activates a widget at all.
        // So without this, every tick would close the list again.
        if (listOpen && pressed)
        {
            if (interactions.chosen.has_value())
            {
                chooseFrom(interactions.chosen->index);

                return false;
            }

            // Anything but the box the list dropped from dismisses it.
            // The box itself falls through to the toggle below.
            // Otherwise pressing it again would dismiss and reopen.
            if (activated != widgets::kGameMenu)
            {
                listOpen = false;

                return true;
            }
        }

        // The overlay menu, on exactly the terms above it.
        // Two lists rather than one, so each is put away on its own.
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

        // The index came off the very list this frame declared.
        // So it names an item by construction.
        // Which is why the last arm needs no test of its own.
        const auto item = static_cast<MenuItem>(index);

        if (item == MenuItem::NewGame)
        {
            commands.newGame();
        }
        else if (item == MenuItem::SaveGame || item == MenuItem::LoadGame)
        {
            // One screen, and both items lead to it.
            // The picker is where a session is written and read back.
            // See IMenuCommands.
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

        // The index came off the very list this frame declared.
        // So it names a view by construction.
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
            // The opposite of what is showing, rather than a flip.
            // PauseState's reason: two presses in one tick agree.
            //
            // The other list is put away on the way.
            // Two open lists over one bar is a picture nobody meant.
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
            // Through the same verb the game menu's own item uses.
            // Leaving for the main menu is one transition.
            // So it is one piece of code, however it was asked for.
            commands.mainMenu();

            // Put away on the way out.
            // So a city entered later is not still wearing it.
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
        // Searched rather than subtracted from kFirstTool.
        // A widget this bar lacks cannot become a tool it has.
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

} // namespace antwika::game
