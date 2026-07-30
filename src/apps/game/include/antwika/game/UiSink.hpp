#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Toolbar.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputState;
    using antwika::ui::Pointer;

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
     * Held state -- the folded pointer, and whether anything has said
     * where it is -- is regenerated from the same events on replay, so
     * it needs no recording of its own.
     */
    class UiSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param camera Zoomed and reset by the buttons. Must outlive
         * this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param codec Decodes the input events off the tick stream.
         * Must outlive this sink.
         * @param toolbar Describes the bar. Must outlive this sink.
         * @param home The camera "reset view" puts back.
         */
        UiSink(
            Camera &camera,
            UiOverlay &overlay,
            const IInputEventCodec &codec,
            const Toolbar &toolbar,
            Camera home);

        UiSink(const UiSink &) = delete;
        UiSink(UiSink &&) = delete;

        UiSink &operator=(const UiSink &) = delete;
        UiSink &operator=(UiSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is folded, resolved against the
         * toolbar and acted on; engine.tick describes the bar once more
         * for the renderer; anything else is ignored.
         * @throws antwika::input::InputError If an input.* event carries
         * a payload of the wrong shape -- raised by the codec, since the
         * wire format is its to police.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed);

        Camera &camera;
        UiOverlay &overlay;
        const IInputEventCodec &codec;
        const Toolbar &toolbar;
        Camera home;

        // Held below the recorder, so a replay folds it again itself.
        InputState state;

        // Until something says where the pointer is, it is nowhere.
        bool located = false;
    };

} // namespace antwika::game
