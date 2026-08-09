#pragma once

#include <cstdint>

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

    class PlaySink final : public ITickEventSink
    {
    public:
        PlaySink(
            PuzzleState &state,
            BoardOverlay &overlay,
            const IInputEventCodec &codec,
            const SudokuScene &scene,
            std::uint64_t solveStepBudget);

        PlaySink(const PlaySink &) = delete;
        PlaySink(PlaySink &&) = delete;

        PlaySink &operator=(const PlaySink &) = delete;
        PlaySink &operator=(PlaySink &&) = delete;

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
        std::uint64_t solveStepBudget;
    };

}
