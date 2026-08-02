#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;

    /**
     * @brief Turns a left click into a tower, inside the tick path.
     *
     * **This application defines no event for placing a tower, and that
     * is the point.**
     * A `--record` run persists the click; which cell it fell in, and
     * whether anything could be built there, are worked out again on
     * replay from the same click.
     * Persisting the placement as well would build two towers per click,
     * the same trap game::GridSink describes for laying a tile.
     *
     * The canvas it maps against is the size the window was *asked* for,
     * never the size a window reports, so which cell a recorded click
     * fell in depends only on the recording and a compile-time constant.
     * The layout is worked out per press rather than once, because the
     * levels of a campaign are not all the same size and the grid a
     * click lands on is whichever one is being fought -- which is state
     * a replay regenerates, so the mapping is regenerated with it.
     * The grid sits below the score bar (see GridLayout), so a click on
     * the bar falls outside the grid and builds nothing -- no sink has
     * to ask the UI whether it covered the pointer.
     *
     * A refused placement is silent.
     * Clicking the road, a cell that already has a tower, or anywhere at
     * all once the campaign is over, is ordinary input rather than an
     * error.
     */
    class TowerPlacementSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param campaign Receives the placements. Must outlive this
         * sink.
         * @param codec Decodes each event. Must outlive this sink.
         * @param canvas Size the grid is laid out against, in pixels.
         */
        TowerPlacementSink(
            Campaign &campaign,
            const IInputEventCodec &codec,
            Size canvas);

        TowerPlacementSink(const TowerPlacementSink &) = delete;
        TowerPlacementSink(TowerPlacementSink &&) = delete;

        TowerPlacementSink &operator=(const TowerPlacementSink &) = delete;
        TowerPlacementSink &operator=(TowerPlacementSink &&) = delete;

        /**
         * @brief Build a tower where a left press landed.
         * @param event The event to fold in; anything that is not a left
         * pointer press is ignored.
         * @throws antwika::input::InputError If the event is one of
         * antwika::input's but its payload is malformed.
         */
        void handle(const TickEvent &event) override;

    private:
        Campaign &campaign;
        const IInputEventCodec &codec;
        Size canvas;
    };

} // namespace antwika::tower_defence
