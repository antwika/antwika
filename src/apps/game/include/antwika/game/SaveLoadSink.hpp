#pragma once

#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadState.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    /**
     * @brief Turns this tick's input into the save screen's presses and
     * typing, and the screen into a picture for the renderer.
     *
     * **The screen defines no event of its own.** A click and a keystroke
     * are the input; they are resolved against the screen's layout here,
     * inside the tick path and downstream of the recorder, and the
     * selection, the typing, the focus and the write itself are all
     * regenerated from them on replay. Persisting "chose option 2" or
     * "saved" alongside the click that caused it would apply it twice --
     * the same trap GridSink describes for placing a tile, and the reason
     * no `ui.*` name may ever exist.
     *
     * It gates itself on the mode rather than being wrapped in a
     * ModeGatedSink, because SaveLoad is its own subject: in any other
     * mode it describes nothing, so the picture it leaves behind is
     * neither redrawn nor paid for.
     *
     * **Loading reads a file inside the tick path, which listing a
     * directory deliberately does not.** The list is fixed for the run
     * -- see listSaveGames() -- but a save file's *contents* can only be
     * read when the click asking for them arrives. A replay therefore
     * reproduces a load exactly as long as the file it names still holds
     * what it held; if somebody overwrites it in between, the replay
     * diverges. That is inherent to a load button rather than something
     * this design gave away: hashing the file into the recording would
     * make such a replay fail loudly instead of quietly, and inlining
     * the whole save into the recording would make every --record file
     * carry every session anybody ever loaded.
     */
    class SaveLoadSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state The screen's own state: the list, the field, the
         * caret, the focus and the message. Must outlive this sink.
         * @param mode The app's mode; asked for MainMenu by "Back" and
         * for CityMap by a load that succeeded. Must outlive this sink.
         * @param overlay The screen's own picture, written every tick it
         * is up. Must outlive this sink.
         * @param input The folded input, holding the event being
         * handled. Must outlive this sink, and must be registered ahead
         * of it.
         * @param scene Describes the screen. Must outlive this sink.
         * @param session Taken from by "Save" and put back by "Load".
         * Must outlive this sink.
         * @param options The run's options, read for the keyboard
         * layout a typed name goes through. Must outlive this sink.
         * @param directory Where saves are kept, which is the one place
         * this sink names a file.
         */
        SaveLoadSink(
            SaveLoadState &state,
            AppModeState &mode,
            UiOverlay &overlay,
            const InputFold &input,
            const SaveLoadScene &scene,
            SessionStore &session,
            const OptionsState &options,
            std::string directory);

        SaveLoadSink(const SaveLoadSink &) = delete;
        SaveLoadSink(SaveLoadSink &&) = delete;

        SaveLoadSink &operator=(const SaveLoadSink &) = delete;
        SaveLoadSink &operator=(SaveLoadSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is resolved against the screen
         * and acted on; engine.tick describes it once more for the
         * renderer; anything else, and anything at all outside the
         * SaveLoad mode, is ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void saveNow();

        void loadNow();

        SaveLoadState &state;
        AppModeState &mode;
        UiOverlay &overlay;
        const InputFold &input;
        const SaveLoadScene &scene;
        SessionStore &session;
        const OptionsState &options;
        std::string directory;
    };

} // namespace antwika::game
