#pragma once

#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Point;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputEvent;
    using antwika::input::InputState;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief Turns this tick's input into edits, and the session into a
     * picture for the renderer.
     *
     * **This application defines no event of its own, and that is the
     * point.** A click is the input; which button it hit, which pixel it
     * landed on and what colour went there are all worked out again on
     * replay from the same click. Persisting the edit as well would
     * paint two pixels for one press, the same trap game::GridSink
     * describes for laying a tile.
     *
     * **Describing and resolving the UI happens here, downstream of the
     * recorder, and never in a renderer**, so no `ui.*` name exists.
     * On engine.tick it describes one last time, so what the renderer
     * paints in that tick is the picture of the state the tick ends
     * with.
     *
     * The toolbar is resolved before the sheet, and a press the UI
     * reports as covered never reaches the image: the sheet is drawn
     * under the whole bar, so without that every button press would also
     * leave a dot on the art.
     *
     * **It holds a translator because the bar is measured from words,
     * and that is what fixes this application's locale.** A press is
     * resolved against a layout whose widths are the labels' own, so
     * the language has to be the same on the machine that recorded a
     * session and on the one replaying it -- which is why main() names
     * i18n::kDefaultLocale and reads one from nowhere else. What the
     * sink puts *into* the state is a StatusMessage rather than a
     * sentence, so no translated string is ever kept.
     *
     * The pointer's folded state lives here rather than in a sink of its
     * own, because this is the only thing in the run that reads input --
     * and it is folded from the events themselves rather than carried,
     * so a replay folds the same edges again.
     */
    class EditorSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state The session it edits. Must outlive this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param store Where a save and a load go. Must outlive this
         * sink.
         * @param codec Decodes the input events off the tick stream.
         * Must outlive this sink.
         * @param translator Words the bar this sink resolves against.
         * Must outlive this sink.
         */
        EditorSink(
            EditorState &state,
            UiOverlay &overlay,
            IAtlasStore &store,
            const IInputEventCodec &codec,
            const Translator &translator);

        EditorSink(const EditorSink &) = delete;
        EditorSink(EditorSink &&) = delete;

        EditorSink &operator=(const EditorSink &) = delete;
        EditorSink &operator=(EditorSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is folded, resolved against the
         * toolbar and then applied to the sheet; engine.tick describes
         * the bar once more for the renderer; anything else is ignored.
         * @throws antwika::input::InputError If an input.* event carries
         * a payload of the wrong shape -- raised by the codec, since the
         * wire format is its to police.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed);

        void act(WidgetId activated);

        /**
         * @brief What one held button does to one pixel.
         *
         * A member pointer rather than a flag, so the segment below is
         * walked by one loop and the two brushes cannot drift apart.
         */
        using Brush = void (EditorState::*)(Point);

        void applyToSheet(const InputEvent &event, Point was, Point at);

        void strokeAlong(Point from, Point to, Brush brush);

        void report(MessageId id, std::string detail);

        void save();

        void load();

        EditorState &state;
        UiOverlay &overlay;
        IAtlasStore &store;
        const IInputEventCodec &codec;
        const Translator &translator;

        // The opening sheet's announcement payload, as this run has it.
        // An announcement that differs is a replay of another sheet.
        // Refused rather than repainted in silence.
        std::string expectedOpening;

        InputState folded;
        std::optional<antwika::time::Tick> foldedTick;
        std::optional<Point> previous;
    };

} // namespace antwika::atlas_editor
