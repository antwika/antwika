#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/DragState.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Holds another system still while the board is being drawn on.
     *
     * Drawing on a board that keeps evolving under the cursor is hard to
     * aim: a cell toggled on one tick may be dead again on the next before
     * the drag reaches the cell beside it. So the generation waits until
     * the button comes back up.
     *
     * A decorator rather than a flag inside LifeSystem, which is what
     * keeps the rule itself unaware that anything could pause it, and what
     * keeps this class usable over any other system that should stand
     * still for the same reason.
     *
     * Only the wrapped system stops. The tick still happens, the board is
     * still committed, and every observer still runs -- which is the whole
     * point, since a pause nobody could see would just look like a freeze.
     * Toggled cells therefore appear as they are drawn.
     *
     * Pausing costs the run nothing in reproducibility: which ticks were
     * paused follows from the recorded presses and releases, so a replay
     * pauses on exactly the same ones.
     */
    class DragPausedSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over what it gates and what gates
         * it.
         * @param inner System to run on the ticks no drag is under way.
         * Must outlive this system.
         * @param drag Consulted every tick. Must outlive this system.
         */
        DragPausedSystem(ISystem &inner, const DragState &drag);

        DragPausedSystem(const DragPausedSystem &) = delete;
        DragPausedSystem(DragPausedSystem &&) = delete;

        DragPausedSystem &operator=(const DragPausedSystem &) = delete;
        DragPausedSystem &operator=(DragPausedSystem &&) = delete;

        /**
         * @brief Run the wrapped system, unless a drag is under way.
         * @param world World handed to the wrapped system untouched.
         * @param tick The tick being run, handed on unchanged.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        antwika::ecs_commons::GatedSystem gate;
    };

} // namespace antwika::life
