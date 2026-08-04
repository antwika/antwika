#pragma once

#include <functional>
#include <utility>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Runs a system only while something else says it may.
     *
     * Three applications wrote this class out with three different
     * questions in the middle of it -- `game`'s `PauseGatedSystem` and
     * `SessionGatedSystem`, `life`'s `DragPausedSystem` -- and the
     * question was the only thing that differed.
     *
     * **Staging nothing is what holds a world still**, which is the
     * whole mechanism: a gated-off system writes no changes, so the
     * commit after its phase finds only what the tick's input did.
     * That is why this is a decorator rather than a branch inside each
     * system: "may this run" is stated once, where the system is
     * registered, and a system itself never learns it was held.
     *
     * The predicate is copied rather than borrowed, so a caller may
     * pass a lambda over the state it reads; every in-tree caller
     * captures a reference to state the composition root owns and
     * outlives the run.
     */
    class GatedSystem final : public ISystem
    {
    public:
        /** @brief Answers whether the inner system may run this tick. */
        using Allows = std::function<bool()>;

        /**
         * @brief Gate a system behind a question.
         * @param inner The system to run when allowed; must outlive
         * this object.
         * @param allows Asked once per tick, before the inner system
         * is offered the world.
         */
        GatedSystem(ISystem &inner, Allows allows)
            : inner(inner), allows(std::move(allows))
        {
        }

        GatedSystem(const GatedSystem &) = delete;
        GatedSystem(GatedSystem &&) = delete;

        GatedSystem &operator=(const GatedSystem &) = delete;
        GatedSystem &operator=(GatedSystem &&) = delete;

        /**
         * @brief Run the inner system if this tick is one it may run.
         * @param world Passed through untouched when it may.
         * @param tick The tick being run.
         */
        void update(World &world, antwika::time::Tick tick) override
        {
            if (!allows())
            {
                return;
            }

            inner.update(world, tick);
        }

    private:
        ISystem &inner;
        Allows allows;
    };

} // namespace antwika::ecs_commons
