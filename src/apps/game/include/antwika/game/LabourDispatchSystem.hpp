#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameConfig.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Sends each house's labourer out with its idle hands.
     *
     * A house with somebody unemployed sends one road-bound labourer at
     * a time, on its own countdown, carrying exactly the number of
     * people the household holds beyond those its Employment ledger
     * says are working -- so a fully employed house sends nobody at
     * all, which is the whole point of the ledger being kept.
     *
     * The labourer roams the roads like any service walker and
     * StaffingSystem empties it into understaffed workplaces; when it
     * runs out, or its budget does, it walks home and whoever it still
     * carries is simply at home again -- the pool is counted from the
     * ledgers and the walker, never stored, so nothing needs handing
     * back.
     *
     * Sending is per house out of its own state, so it reads the view
     * directly exactly as PopulationSystem's sending does; the walker
     * limit is the one contended amount, and the sends are walked out
     * of a map in ascending Cell order for SpawnSystem's reason.
     */
    class LabourDispatchSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the roads it spawns onto.
         * @param paths Consulted for a road beside each door; must
         * outlive this system.
         * @param config The dispatch period and the walker cap; copied,
         * so no lifetime rule attaches to it.
         */
        LabourDispatchSystem(
            const PathIndex &paths, GameConfig config) noexcept;

        LabourDispatchSystem(const LabourDispatchSystem &) = delete;
        LabourDispatchSystem(LabourDispatchSystem &&) = delete;

        LabourDispatchSystem &operator=(const LabourDispatchSystem &)
            = delete;
        LabourDispatchSystem &operator=(LabourDispatchSystem &&) = delete;

        /**
         * @brief Count each house down and send what is due.
         * @param world The world to read from and stage changes into.
         * @param tick The tick being processed; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GameConfig config;
    };

} // namespace antwika::game
