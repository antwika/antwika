#pragma once

#include <antwika/ecs/SystemScheduler.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/event/ITimedEventSink.hpp>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::SystemScheduler;
    using antwika::ecs::World;
    using antwika::event::ITimedEventSink;
    using antwika::event::TimedEvent;

    /**
     * @brief Drives the ECS world from the same TimedEvent stream that
     * carries this application's custom events.
     *
     * Reacts to the engine's built-in tick event by committing any cell
     * toggled this tick, then running one generation of LifeSystem through
     * scheduler. Reacts to events::kToggleCell by flipping the named
     * cell's Cell::alive, staged for that commit. This is the same
     * ITimedEventSink mechanism apps/game's GameStateReducer uses to fold
     * events into a plain struct, applied here to an ECS World instead.
     */
    class BoardSink final : public ITimedEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param world World toggled cells are staged into and committed.
         * @param grid Maps a toggle event's "x,y" payload to an entity.
         * @param scheduler Run once per tick, after the commit above.
         */
        BoardSink(World &world, const Grid &grid, SystemScheduler &scheduler);

        /**
         * @brief Apply a timed event's effect to the referenced World.
         * @param event kTick commits and advances one generation;
         * kToggleCell flips one cell, staged for the next commit.
         */
        void handle(const TimedEvent &event) override;

    private:
        World &world;
        const Grid &grid;
        SystemScheduler &scheduler;
    };

} // namespace antwika::life
