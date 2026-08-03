#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Burns every fire down, and sends a fireman to each one.
     *
     * The two halves of what a fire does on its own: it runs out --
     * kBurnDurationTicks after ignition the ruin turns to debris
     * whether or not anybody came -- and it calls for help, tasking
     * the nearest free fireman with a FireCall.
     *
     * **One fireman per fire, and the nearest one.** A fire nobody is
     * coming to is given the free fireman with the shortest walk as
     * the crow flies, ties broken by ascending Cell and then Entity,
     * so a replay and a restore task the same man. Free means not
     * already called -- a fireman is a service walker, and its kind
     * never carries an errand or a journey. The fires are answered in
     * ascending Cell order, so two fires contending for one fireman
     * is a total order rather than a view's.
     *
     * **Putting a fire out is WalkerSystem's, not this system's**, and
     * the split is CoverageSystem's exactly: arrival is a fact about a
     * walker's step, worked out where every other arrival is, and the
     * two systems write Ruin from different phases so neither write
     * can undo the other. This system runs in a phase of its own after
     * the walk, so a fire put out this tick is already debris here and
     * is neither counted down nor assigned to.
     *
     * Nothing here is an event: a fire is a pure function of risk,
     * which is a function of coverage, which is a function of the
     * clicks that built the district -- so a recorder would write it
     * beside those clicks and a replay would burn twice.
     */
    class RuinSystem final : public ISystem
    {
    public:
        /**
         * @brief Count every fire down, and task the idle firemen.
         * @param world Read for the ruins and the walkers; the calls
         * and the burn-downs are staged into it.
         * @param tick The tick being processed; unused, because the
         * burn countdown is per ruin for the reason Building's are.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::game
