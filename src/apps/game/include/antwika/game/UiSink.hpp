#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/MenuModalScene.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief Turns this tick's input into toolbar presses, and the
     * toolbar into a picture for the renderer.
     *
     * **Describing and resolving the UI happens here, downstream of the
     * recorder, on purpose.** A replay carries the click and works out
     * which button it hit all over again; this app therefore defines no
     * event for a button at all, and no `ui.*` name is ever persisted.
     * Persisting the press *and* what it activated would zoom twice for
     * one click, the same trap GridSink describes for placing a tile.
     *
     * Registered before GridSink, so a press has been resolved against
     * the toolbar by the time the grid sees it, and UiOverlay can answer
     * whether the click was the toolbar's. On engine.tick it describes
     * one last time, so what the renderer paints in that tick is the
     * picture of the state the tick ends with.
     *
     * The canvas comes off the overlay, which is the size the window
     * was asked for rather than the size one reports -- see UiOverlay.
     *
     * The pause button is the same rule read out loud: pressing it
     * toggles PauseState here, in the tick path, so a replay pauses on
     * precisely the ticks the live run paused on. Nothing about a pause
     * is persisted, and no `game.pause` event exists to persist.
     *
     * What the pointer is doing comes from the shared InputFold, which
     * is registered ahead of this sink and holds the app's one answer --
     * regenerated from the same events on replay, so it needs no
     * recording of its own.
     *
     * **The menu modal is this sink's too, and it is simulation state**
     * in exactly the sense PauseState, the camera and the selected tool
     * are: whether it is up decides what a recorded press *means*, so it
     * is regenerated from the recorded F10 and the recorded clicks
     * rather than recorded itself. No `game.*` or `ui.*` event exists
     * for opening, closing or leaving by it. It lives here rather than
     * in a shared state object because this sink is the only thing that
     * reads it: what it covers is reported through UiOverlay like
     * everything else the UI covers, and what it draws is appended to
     * the same picture the bar goes into.
     *
     * Three rules are worth stating outright.
     *
     * **A press is resolved against the modal alone while it is up**, so
     * a toolbar button cannot be pressed through it, and the modal's
     * commands are appended after the bar's so paint order puts it in
     * front -- which is how "on top" is said where antwika::gfx offers
     * no depth. The scrim's fill is what makes UiOverlay report the
     * pointer as covered wherever it is, so GridSink's existing rule
     * keeps the press off the city with no second mechanism.
     *
     * **Opening the modal ends a road drag in progress, and that drag
     * lays nothing at all.** What a drag lays is what its release said,
     * and a release arriving over the modal never said it; leaving the
     * gesture alive would also leave a route being planned against a
     * city nobody can see.
     *
     * **Opening the modal holds the run**, exactly as entering a city
     * does, and closing it does not let the run go again -- hold()
     * rather than toggle() for CityEntrySink's reason, and no release
     * for the same one, since the way out is the pause button it always
     * was. That also settles the drag: the modal's hold supersedes the
     * drag's, so a drag ended here never resumes anything.
     */
    class UiSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param camera Zoomed and reset by the buttons. Must outlive
         * this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param input The folded input, holding the event being
         * handled. Must outlive this sink, and must be registered ahead
         * of it.
         * @param toolbar Describes the bar. Must outlive this sink.
         * @param pause Toggled by the pause button, held by the modal
         * opening, and read to label the button. Must outlive this sink.
         * @param mode The app's mode; asked for MainMenu by the modal's
         * own item. Must outlive this sink.
         * @param drag The road drag opening the modal ends. Written here
         * and by GridSink, and by nothing else. Must outlive this sink.
         * @param modal Describes the menu modal. Must outlive this sink.
         * @param home The camera "reset view" puts back.
         */
        UiSink(
            Camera &camera,
            UiOverlay &overlay,
            const InputFold &input,
            const Toolbar &toolbar,
            PauseState &pause,
            AppModeState &mode,
            RoadDrag &drag,
            const MenuModalScene &modal,
            Camera home);

        UiSink(const UiSink &) = delete;
        UiSink(UiSink &&) = delete;

        UiSink &operator=(const UiSink &) = delete;
        UiSink &operator=(UiSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is resolved against the toolbar
         * and acted on; engine.tick describes the bar once more for the
         * renderer; anything else is ignored.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Check whether the menu modal is up.
         * @return True between opening it and closing it.
         */
        [[nodiscard]] bool menuOpen() const noexcept;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        [[nodiscard]] Frame describeNow(bool pressed) const;

        void refreshAndAct(bool pressed);

        void actOnBar(WidgetId activated);

        void actOnModal(WidgetId activated);

        void openModal();

        void selectFrom(WidgetId activated);

        Camera &camera;
        UiOverlay &overlay;
        const InputFold &input;
        const Toolbar &toolbar;
        PauseState &pause;
        AppModeState &mode;
        RoadDrag &drag;
        const MenuModalScene &modal;
        Camera home;

        // Whether the menu modal is up, which is simulation state.
        // Written here, inside the tick path, and read nowhere else.
        // A replay regenerates it from the F10 and the clicks it holds.
        bool modalOpen = false;

        // The tick the bar reports, off the event being handled.
        // Every tick event carries the tick it belongs to.
        // So this is the simulation's count rather than a second one.
        // A replay hands over the same numbers in the same order.
        antwika::time::Tick tick = 0;
    };

} // namespace antwika::game
