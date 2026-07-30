#pragma once

#include <functional>
#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/InputFold.hpp"
#include "antwika/game/MainMenu.hpp"
#include "antwika/game/MenuState.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief The key that puts the menu up, and takes it down again.
     *
     * One key for both, whether or not a game is under way, so there is
     * no state in which the menu cannot be reached.
     */
    inline constexpr antwika::input::Key kMenuKey =
        antwika::input::Key::F10;

    /**
     * @brief Turns this tick's input into menu presses, and the menu
     * into a picture for the renderer.
     *
     * **Describing and resolving the UI happens here, downstream of the
     * recorder, on purpose.** A replay carries the F10 press and the
     * click and works out which entry they hit all over again; this app
     * therefore defines no event for the menu at all, and no `ui.*` name
     * is ever persisted. Persisting the press *and* what it activated
     * would act on one click twice, the same trap UiSink describes for
     * the toolbar and GridSink for laying a tile.
     *
     * **The menu is modal.** It fills the canvas, so while it is up the
     * overlay reports every pixel as covered and GridSink skips the
     * click -- and the sink handed to it as `whenClosed` (the toolbar's
     * UiSink, in the shipped wiring) is not called at all. A bar drawn
     * under a menu that covers it must not still be pressable through
     * it, and skipping the sink is the only way to say so without the
     * bar having to know what a menu is.
     *
     * Registered **before GridSink**, so a press has been resolved
     * against the menu by the time the grid sees it. On engine.tick it
     * describes one last time, so what the renderer paints in that tick
     * is the picture of the state the tick ends with.
     *
     * The canvas comes off the overlay, which is the size the window was
     * *asked* for rather than the size one reports -- see UiOverlay.
     *
     * What the pointer is doing comes from the shared InputFold, which
     * is registered ahead of this sink and holds the app's one answer --
     * regenerated from the same events on replay, so it needs no
     * recording of its own.
     *
     * Loading and saving a replay are reported through MenuState rather
     * than done here: there is no file dialog anywhere in this project,
     * so the paths are the ones `--record` / `--replay` already gave the
     * application, and acting on them is the application's business.
     */
    class MenuSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state Opened, closed and activated by this sink. Must
         * outlive it.
         * @param overlay Written whenever the menu is up, and emptied
         * when it goes away. Must outlive this sink.
         * @param input The folded input, holding the event being
         * handled. Must outlive this sink, and must be registered ahead
         * of it.
         * @param menu Describes the menu. Must outlive this sink.
         * @param whenClosed The sink to pass the event on to while the
         * menu is down -- the toolbar's, in the shipped wiring. Must
         * outlive this sink. Left out, nothing is passed on and this
         * sink is simply the menu.
         */
        MenuSink(
            MenuState &state,
            UiOverlay &overlay,
            const InputFold &input,
            const MainMenu &menu,
            std::optional<std::reference_wrapper<ITickEventSink>>
                whenClosed = std::nullopt);

        MenuSink(const MenuSink &) = delete;
        MenuSink(MenuSink &&) = delete;

        MenuSink &operator=(const MenuSink &) = delete;
        MenuSink &operator=(MenuSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An F10 press toggles the menu; any other input.*
         * event is resolved against an open menu and acted on;
         * engine.tick describes an open menu once more for the renderer.
         * Anything arriving while the menu is down is passed on to the
         * sink this one was given, if it was given one.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void react(const antwika::input::InputEvent &decoded);

        void refreshAndAct(bool pressed);

        void act(WidgetId activated);

        MenuState &state;
        UiOverlay &overlay;
        const InputFold &input;
        const MainMenu &menu;
        std::optional<std::reference_wrapper<ITickEventSink>> whenClosed;

        std::optional<antwika::time::Tick> handledTick;
    };

} // namespace antwika::game
