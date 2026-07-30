#pragma once

#include <map>
#include <optional>
#include <set>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/Position.hpp>

#include "antwika/life/BoardLayout.hpp"
#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::Entity;
    using antwika::ecs::World;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::input::Position;

    /**
     * @brief Turns pointer input into toggled cells, so a board can be
     * drawn on by dragging across it.
     *
     * A sink beside BoardSink rather than a change to it: BoardSink folds
     * this application's own life.toggle_cell events, and this one folds
     * antwika::input's, without either having to know the other exists.
     *
     * Translating input into application meaning belongs here, downstream
     * of the recorder, and not in the source that reads the device. What
     * a `--record` run persists is therefore the click, and the toggle is
     * regenerated from it on replay -- persisting the toggle instead would
     * be storing a derived event, and a replay is meant to hold only what
     * came from outside.
     *
     * The canvas it maps against is the size the window was *asked* for,
     * not the size a window currently reports. That is what keeps a
     * recorded session reproducible under a different backend: which cell
     * a click fell in then depends only on the recorded position and a
     * compile-time constant, rather than on how some window manager sized
     * a window on the day. The cost is that clicks land by the original
     * geometry if a window is resized out from under it, which is why the
     * window this drives is not resizable.
     *
     * Held state -- which cells this drag has already visited, and what
     * this tick has staged -- is folded from the same events rather than
     * read from a device, so it is regenerated identically on replay. A
     * cell is toggled at most once per drag, so dragging back across a
     * cell leaves it as the drag first made it instead of flickering.
     *
     * Whether a button is down goes into the shared DragState rather than
     * staying private here, because the generation has to stand still
     * while the board is being drawn on -- see DragPausedSystem. A press
     * that lands outside the board still starts a drag, and so still
     * pauses: what pauses is holding the button, not hitting a cell.
     */
    class PointerToggleSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param world World toggled cells are staged into. Must outlive
         * this sink.
         * @param grid Maps a cell coordinate to an entity. Must outlive
         * this sink.
         * @param codec Decodes each event. Must outlive this sink.
         * @param canvas Size the board is laid out against, in pixels.
         * @param drag Told when a drag starts and finishes. Must outlive
         * this sink.
         */
        PointerToggleSink(
            World &world,
            const Grid &grid,
            const IInputEventCodec &codec,
            Size canvas,
            DragState &drag);

        PointerToggleSink(const PointerToggleSink &) = delete;
        PointerToggleSink(PointerToggleSink &&) = delete;

        PointerToggleSink &operator=(const PointerToggleSink &) = delete;
        PointerToggleSink &operator=(PointerToggleSink &&) = delete;

        /**
         * @brief Apply a tick event's effect to the referenced World.
         *
         * A left press starts a drag and toggles the cell under the
         * pointer; a move while dragging toggles each further cell it
         * reaches; a left release ends the drag. engine.tick is where
         * what this staged stops being its business, since that is when
         * BoardSink commits it and runs a generation over it. Every other
         * event, input or not, is ignored.
         *
         * @param event The event to fold in.
         * @throws antwika::input::InputError If the event is one of
         * antwika::input's but its payload is malformed.
         */
        void handle(const TickEvent &event) override;

    private:
        void toggleAt(Position position);

        World &world;
        const Grid &grid;
        const IInputEventCodec &codec;
        DragState &drag;
        std::optional<BoardLayout> layout;
        std::set<Entity> visited;
        std::map<Entity, bool> staged;
    };

} // namespace antwika::life
