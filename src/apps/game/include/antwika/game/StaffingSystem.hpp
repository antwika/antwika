#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Tuning.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Moves workforce off labourers into workplaces, and lets it
     * drift home again.
     *
     * **The one writer of both ledgers.** Staff on a workplace and
     * Employment on a house say one fact from two ends -- these people
     * work there -- and a single writer is what keeps the two agreeing;
     * see Staff.hpp.
     *
     * Three passes, every tick:
     *
     * 1. **Tend**: every workplace that wants workers and has no Staff
     * ledger yet is given an empty one, so "absent means fully staffed"
     * ends the moment this system runs. Every ledger entry naming a
     * dead house, and every job holding naming a dead workplace, is
     * dropped -- a demolition needs no bookkeeping of its own here.
     *
     * 2. **Decay**: each staffed workplace counts its own ticks down,
     * and on expiry one person leaves the lowest occupied slot and is
     * handed back to their house's ledger. A building nothing staffs
     * for long enough therefore runs down to nobody, which is what
     * slows and finally stops its own walker -- workedPeriod() already
     * says how staffing becomes a rate.
     *
     * 3. **Transfer**: each labourer standing on a road hands people to
     * the understaffed workplaces beside it, in ascending (Cell,
     * Entity) order out of a map, since workforce is a limited amount
     * two walkers may contend for. A labourer that runs out is turned
     * for home by zeroing its roaming budget; WalkerSystem does the
     * walking.
     *
     * Every changed component is accumulated and written once, which is
     * the double-buffer rule every system here follows.
     */
    class StaffingSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over its decay period.
         * @param tuning The staff decay period; copied, so no lifetime
         * rule attaches to it.
         */
        explicit StaffingSystem(Tuning tuning);

        StaffingSystem(const StaffingSystem &) = delete;
        StaffingSystem(StaffingSystem &&) = delete;

        StaffingSystem &operator=(const StaffingSystem &) = delete;
        StaffingSystem &operator=(StaffingSystem &&) = delete;

        /**
         * @brief Tend the ledgers, decay staffing, move workforce.
         * @param world The world to read from and stage changes into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        Tuning tuning;
    };

} // namespace antwika::game
