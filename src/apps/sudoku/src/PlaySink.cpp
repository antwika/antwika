#include "antwika/sudoku/PlaySink.hpp"

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

    using antwika::app::locates;
    using antwika::gfx::Point;
    using antwika::input::InputEvent;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;

    namespace
    {
        [[nodiscard]] bool isLeftPress(const InputEvent &event) noexcept
        {
            const auto *pressed =
                std::get_if<PointerButtonPressed>(&event);

            return pressed != nullptr
                   && pressed->button == MouseButton::Left;
        }

        // A repeat is the window system saying a key is still held.
        // Writing a digit is something somebody does once per press.
        // So a repeat is stepped over.
        // input::KeyPressed::repeat exists for that distinction.
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
    } // namespace

    PlaySink::PlaySink(
        PuzzleState &state,
        BoardOverlay &overlay,
        const IInputEventCodec &codec,
        const SudokuScene &scene)
        : state(state), overlay(overlay), codec(codec), scene(scene)
    {
    }

    void PlaySink::handle(const TickEvent &event)
    {
        // A tick's edges are cleared on the next tick's first event.
        // Clearing at the end of a tick would need this sink last.
        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            // Described again here, for the renderer about to paint.
            // So what it draws is the state this tick ends with.
            // BoardSink's scripted events included.
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

        // What a press or a keystroke just did is not in that picture.
        // So it is described once more, and the second one is drawn.
        // The same remedy ui::Context::finish() spells out.
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
            state.solve();
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
        // An id every frame declares, so the first fallback is dead.
        // A layout with no room for a grid answers no square.
        // So neither needs a branch of its own here.
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

} // namespace antwika::sudoku
