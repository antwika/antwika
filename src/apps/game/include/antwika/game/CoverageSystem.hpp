#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Refreshes each building's coverage from the walkers
     * passing it, and decays what nobody refreshed.
     *
     * **This is what a service walker is for, and it replaced a walker
     * handing a number over.** A fireman used to subtract a fixed amount
     * of risk from whatever he walked past, which was coverage in
     * disguise: it made "somebody came recently" the only thing that
     * mattered, but expressed it as an amount rather than as a state
     * with a lifetime. Saying it once, as a per-service countdown, gets
     * the well, the doctor, the fire station and the engineer's post out
     * of the delivery code entirely and gives a housing rule something
     * to read that is not a subtraction it has to reverse.
     *
     * It runs in a phase of its own, after the walk, so it sees where
     * this tick left every walker.
     *
     * **What coverage then does about risk is BuildingSystem's, not
     * this system's**, and that split is deliberate rather than
     * incidental. `risk` is a field of Building, and BuildingSystem's
     * age() is the one place a building's countdowns advance and the
     * one place a lost building is worked out from the amounts this
     * tick produced. Raising risk here would make two systems in two
     * phases write one component, and would put a tick between the
     * risk that finished a building and the pass that notices. So this
     * system owns Coverage and nothing else, and BuildingSystem asks
     * coverageOf() which way to step.
     *
     * **Nothing here depends on which entity a view happens to walk
     * first.** A top-up is std::max against kCoverageFull, so two
     * walkers of one service beside one building leave exactly what one
     * of them leaves; a decay is read out of the building's own
     * component and touches nothing else. Those are two of the three
     * shapes section 6 of the round-one plan allows a view() loop to
     * have -- idempotent, commutative, or independent per entity.
     */
    class CoverageSystem final : public ISystem
    {
    public:
        /**
         * @brief Decay every coverage, then refresh it from the
         * walkers standing beside each building.
         *
         * A building with no Coverage component is given one the first
         * time a walker reaches it, and never before: an absent
         * component already means uncovered -- see coverageOf() -- so
         * handing every building an all-zero one would say nothing and
         * write it to every save file.
         *
         * @param world The world to read from and stage changes into.
         * @param tick The tick being processed; unused, because every
         * countdown here is per building for the reason Building's are.
         */
        void update(World &world, antwika::time::Tick tick) override;
    };

} // namespace antwika::game
