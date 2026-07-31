#pragma once

#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/OptionChoice.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"

namespace antwika::ui_demo
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputState;
    using antwika::ui::Interactions;
    using antwika::ui::OptionChoice;
    using antwika::ui::TextEdit;
    using antwika::ui::WidgetId;

    /**
     * @brief Turns this tick's input into the showcase's presses,
     * choices and typing, and the showcase into a picture.
     *
     * **Describing and resolving the UI happens here, downstream of the
     * recorder, and never in a renderer.**
     * A click and a keystroke are the input; which widget they landed on
     * is worked out again from them on replay, and so are the page
     * showing, both lists' open flags, the selections, the focus and
     * every character in the field.
     * Persisting "chose page 4" alongside the click that chose it would
     * apply it twice -- the same trap game::GridSink describes for
     * placing a tile, and the reason no `ui.*` event name may ever
     * exist.
     *
     * It folds the input itself rather than sharing a fold, because it
     * is the only sink here that reads any: game::InputFold exists for
     * an application whose sinks would otherwise each keep their own
     * copy of one truth, and this application has one such sink.
     */
    class DemoSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state The page, the lists, the field, the caret, the
         * focus and the message. Must outlive this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param codec Decodes the input events off the tick stream.
         * Must outlive this sink.
         * @param scene Describes the showcase. Must outlive this sink.
         */
        DemoSink(
            DemoState &state,
            DemoOverlay &overlay,
            const IInputEventCodec &codec,
            const DemoScene &scene);

        DemoSink(const DemoSink &) = delete;
        DemoSink(DemoSink &&) = delete;

        DemoSink &operator=(const DemoSink &) = delete;
        DemoSink &operator=(DemoSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is resolved against the
         * showcase and acted on; engine.tick describes it once more for
         * the renderer; anything else is ignored.
         * @throws antwika::input::InputError If an input.* event carries
         * a payload of the wrong shape -- raised by the codec, since the
         * wire format is its to police.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void choose(const OptionChoice &choice);

        void edit(const TextEdit &change);

        void press(WidgetId activated);

        DemoState &state;
        DemoOverlay &overlay;
        const IInputEventCodec &codec;
        const DemoScene &scene;

        InputState folded;
        std::optional<antwika::time::Tick> foldedTick;
        bool located = false;
    };

} // namespace antwika::ui_demo
