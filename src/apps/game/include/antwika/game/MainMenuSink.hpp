#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MainMenuScene.hpp"
#include "antwika/game/OptionsScene.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Pointer;

    /**
     * @brief Turns this tick's input into menu presses, and the menu into
     * a picture for the renderer.
     *
     * **The menu defines no event of its own.** A click is the input; it
     * is resolved against the menu's layout here, inside the tick path
     * and downstream of the recorder; the mode change and the stop are
     * regenerated from it on replay. Persisting a "started a new game"
     * event alongside the click that caused it would apply it twice, the
     * same trap GridSink describes for placing a tile -- and no `ui.*`
     * name may ever exist.
     *
     * Registered *before* the grid's sink, so a press is resolved against
     * the menu before the world could see it -- and the mode change it
     * asks for lands at the tick boundary, so the very click that leaves
     * the menu cannot also be read as a click on the grid it reveals.
     * See AppMode.hpp.
     *
     * It gates itself on the mode rather than being wrapped in a
     * ModeGatedSink, because MainMenu is its own subject: in any other
     * mode it describes nothing, so the picture it leaves behind is
     * neither redrawn nor paid for.
     *
     * Quit ends the run by signalling the loop's stop directly rather
     * than by putting an engine.stop on the wire, for the same reason:
     * the recording holds the click, and the stop follows from it.
     *
     * **The options screen is this sink's too**, because it is this
     * screen with something else on it: which of the two is up is a flag
     * in OptionsState, both are described into the one overlay, and
     * MainMenuScene::draw() paints whichever it was handed. A mode of
     * its own would have wanted a fourth overlay and a fourth branch in
     * the renderer for a card that is the menu's other face; a second
     * sink over one overlay would have had two writers of one picture.
     *
     * That screen is also the one place a *key press* means something
     * here, and it means one thing: the key an action is being bound to.
     * That is resolved in the tick path like every click, so a replay
     * rebinds exactly what the run rebound.
     */
    class MainMenuSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param mode The app's mode; asked for Playing by "New Game".
         * Must outlive this sink.
         * @param overlay The menu's own picture, written every tick the
         * menu is up. Must outlive this sink.
         * @param input The folded input, holding the event being
         * handled. Must outlive this sink, and must be registered ahead
         * of it.
         * @param scene Describes the menu. Must outlive this sink.
         * @param stop Signalled by "Quit"; the same StopSignal
         * EngineLoop is watching. Must outlive this sink.
         * @param options Whether the key bindings are showing, which
         * action is waiting for a key, and what the bindings are. Must
         * outlive this sink.
         * @param optionsScene Describes the key bindings. Must outlive
         * this sink.
         * @param locale The language the run is in, which a press on the
         * options screen asks to change. Must outlive this sink.
         */
        MainMenuSink(
            AppModeState &mode,
            UiOverlay &overlay,
            const InputFold &input,
            const MainMenuScene &scene,
            ITickEventSink &stop,
            OptionsState &options,
            const OptionsScene &optionsScene,
            LocaleState &locale);

        MainMenuSink(const MainMenuSink &) = delete;
        MainMenuSink(MainMenuSink &&) = delete;

        MainMenuSink &operator=(const MainMenuSink &) = delete;
        MainMenuSink &operator=(MainMenuSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is resolved against the menu and
         * acted on; engine.tick describes the menu once more for the
         * renderer; anything else, and anything at all outside the
         * MainMenu mode, is ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(const TickEvent &event, bool pressed);

        void refreshOptions(bool pressed);

        AppModeState &mode;
        UiOverlay &overlay;
        const InputFold &input;
        const MainMenuScene &scene;
        ITickEventSink &stop;
        OptionsState &options;
        const OptionsScene &optionsScene;
        LocaleState &locale;
    };

} // namespace antwika::game
