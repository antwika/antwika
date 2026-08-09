#include "antwika/sudoku/PlaySink.hpp"

#include <cstdint>

#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/sudoku/BoardLayout.hpp"
#include "antwika/sudoku/KeyMapping.hpp"
#include "antwika/sudoku/Widgets.hpp"

namespace antwika::sudoku
{

    using antwika::app::isLeftPress;
    using antwika::app::locates;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    namespace
    {
        [[nodiscard]] std::optional<int> digitTyped(
            const InputEvent &event) noexcept
        {
            const auto *key = std::get_if<KeyPressed>(&event);

            if (key == nullptr || key->repeat)
            {
                return std::nullopt;
            }

            return digitFor(key->key);
        }
    }

    PlaySink::PlaySink(
        PuzzleState &state,
        BoardOverlay &overlay,
        const IInputEventCodec &codec,
        const SudokuScene &scene,
        std::uint64_t solveStepBudget)
        : state(state),
          overlay(overlay),
          codec(codec),
          scene(scene),
          solveStepBudget(solveStepBudget)
    {
    }

    void PlaySink::handle(const TickEvent &event)
    {
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            refreshAndAct(false, std::nullopt);
            return;
        }

        const auto decoded = codec.decode(event.event);

        if (!decoded.has_value())
        {
            return;
        }

        located = located || locates(*decoded);
        folded.apply(*decoded);

        refreshAndAct(isLeftPress(*decoded), digitTyped(*decoded));
    }

    Pointer PlaySink::pointerNow(const bool pressed) const
    {
        const auto &mouse = folded.mouse();

        return Pointer{
            .position =
                located ? std::optional<Point>{antwika::app::asPoint(
                              mouse.position())}
                        : std::nullopt,
            .down = mouse.isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void PlaySink::refreshAndAct(
        const bool pressed, const std::optional<int> typed)
    {
        auto frame = scene.describe(
            overlay.canvas(), pointerNow(pressed), state);

        if (act(frame, typed))
        {
            frame = scene.describe(
                overlay.canvas(), pointerNow(pressed), state);
        }

        overlay.set(std::move(frame.commands));
    }

    bool PlaySink::act(
        const Frame &frame, const std::optional<int> typed)
    {
        if (typed.has_value())
        {
            state.enter(*typed);
            return true;
        }

        if (frame.interactions.activated == widgets::kSolve)
        {
            state.solve({.maxSteps = solveStepBudget});
            return true;
        }

        if (frame.interactions.activated == widgets::kBoard)
        {
            return pickSquare(frame);
        }

        return false;
    }

    bool PlaySink::pickSquare(const Frame &frame)
    {
        const auto layout =
            layoutFor(frame.rects.find(widgets::kBoard)
                          .value_or(antwika::gfx::Rect{}))
                .value_or(BoardLayout{});

        const auto position = folded.mouse().position();
        const auto square = cellAt(layout, position.x, position.y);

        if (!square.has_value())
        {
            return false;
        }

        state.select(*square);
        return true;
    }

}
