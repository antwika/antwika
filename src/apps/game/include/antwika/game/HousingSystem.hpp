#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/Tuning.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Grows and shrinks a house by what has been reaching it.
     *
     * **This is the rule the rest of the city is arranged to satisfy.**
     * A house holding the next level's requirements -- the services it
     * names still reaching it, the goods it names on its shelves, and the
     * ground under it at or above its desirability -- for kEvolvePeriodTicks
     * without a break grows into that level. One falling short of the
     * requirements of the level it is already on, for kDevolvePeriodTicks,
     * drops back. That is what turns a district into a feedback loop
     * rather than a set of buildings that happen to be near each other.
     *
     * **Per house, out of that house's own state, and that is the whole
     * determinism argument.** Nothing here is split between houses and
     * nothing one house does changes what another is owed, so no order
     * over entities is observable and this may read ecs::View directly --
     * whose order is "whichever storage has the fewest entities" and is
     * nobody's to name. Two houses meeting the same requirements at the
     * same tick both grow, and neither takes anything from the other.
     *
     * **There is deliberately no merging of houses into blocks.** It is
     * the one housing rule that would need a total order over
     * *neighbours* rather than over one entity's own fields, and putting
     * it in would have doubled this workstream for a picture.
     *
     * **A phase of its own, "settle", after the goods have moved.** A
     * phase is where the World's buffers swap, so two systems in one
     * phase both read what the last swap left; this reads the stock a
     * seller delivered in the same tick rather than the previous tick's,
     * which is what keeps "held the requirements for a countdown" from
     * being off by one delivery. Nothing else in that phase writes a
     * Household, and this writes nothing else -- it never touches
     * Building at all, which is what lets a later workstream put its own
     * systems beside it.
     *
     * Nothing here is a persisted event, and the tempting one is
     * `game.house_evolved`, which reads like a notification worth
     * recording. It is a pure function of coverage, stock and
     * desirability, every one of which a replay regenerates, so a
     * recorder would write it beside the click that built the market and
     * a replay would evolve the house twice.
     */
    class HousingSystem final : public ISystem
    {
    public:
        /**
         * @brief Build the system over the field it judges ground by.
         * @param desirability The field, rebuilt every tick by
         * DesirabilitySystem in the phase before this one. Must outlive
         * this object.
         * @param tuning The evolve and devolve periods; copied, so no
         * lifetime rule attaches to it.
         */
        HousingSystem(
            const DesirabilityField &desirability, Tuning tuning);

        HousingSystem(const HousingSystem &) = delete;
        HousingSystem(HousingSystem &&) = delete;

        HousingSystem &operator=(const HousingSystem &) = delete;
        HousingSystem &operator=(HousingSystem &&) = delete;

        /**
         * @brief Count every house toward its next level or its last one.
         *
         * A house is given a Household the first time it has something
         * to say and never before, exactly as a building is given a
         * Coverage the first time somebody reaches it: a default-valued
         * component and no component at all mean the same thing, and
         * writing one anyway would put a member in every save file for
         * every house that has never done anything.
         *
         * @param world Read for the houses and their stock, staged into
         * with their households.
         * @param tick The tick being processed; unused, deliberately --
         * the countdowns are per house for the reason the class comment
         * gives.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const DesirabilityField &desirability;
        Tuning tuning;
    };

} // namespace antwika::game
