#pragma once

#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/sudoku/BoardOverlay.hpp"
#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/SudokuScene.hpp"

namespace antwika::sudoku
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputState;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;

    /**
     * @brief Turns this tick's input into a picked square, a typed
     * digit or a solve, and the session into a picture.
     *
     * **Describing and resolving the UI happens here, downstream of the
     * recorder, and never in a renderer.**
     * A click and a keystroke are the input; which widget or which
     * square they landed on is worked out again from them on replay,
     * and so are the digit that ended up in the square, the squares a
     * solve filled and whether the grid is finished.
     * Persisting "square (4,2) became a 7" alongside the keystroke that
     * put it there would apply it twice, which is the same trap
     * game::GridSink describes for placing a tile and the reason no
     * `ui.*` event name may ever exist.
     *
     * One hit-test decides everything a press means. The bar's button
     * and the board's area are both named widgets of the same frame, so
     * antwika::ui reports which of them the press was topmost over, and
     * only a press the board answered for is mapped to a square. A
     * second, independent test of "was this over the bar" is precisely
     * the drift ui::Frame::rects exists to prevent.
     */
    class PlaySink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param state The puzzle, the selection and the last thing
         * said. Must outlive this sink.
         * @param overlay Written every tick. Must outlive this sink.
         * @param codec Decodes the input events off the tick stream.
         * Must outlive this sink.
         * @param scene Describes the picture. Must outlive this sink.
         */
        PlaySink(
            PuzzleState &state,
            BoardOverlay &overlay,
            const IInputEventCodec &codec,
            const SudokuScene &scene);

        PlaySink(const PlaySink &) = delete;
        PlaySink(PlaySink &&) = delete;

        PlaySink &operator=(const PlaySink &) = delete;
        PlaySink &operator=(PlaySink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event An input.* event is resolved against the picture
         * and acted on; engine.tick describes it once more for the
         * renderer; anything else is ignored.
         * @throws antwika::input::InputError If an input.* event
         * carries a payload of the wrong shape -- raised by the codec,
         * since the wire format is its to police.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, std::optional<int> typed);

        [[nodiscard]] bool act(
            const Frame &frame, std::optional<int> typed);

        [[nodiscard]] bool pickSquare(const Frame &frame);

        PuzzleState &state;
        BoardOverlay &overlay;
        const IInputEventCodec &codec;
        const SudokuScene &scene;

        InputState folded;
        std::optional<antwika::time::Tick> foldedTick;
        bool located = false;
    };

} // namespace antwika::sudoku
