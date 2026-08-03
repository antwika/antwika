#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief How many ticks a second of game time is.
     *
     * An assumption stated once rather than a fact the code can read.
     * The pacing lives in the composition root, and a headless run is
     * not paced at all, so nothing here can ask what rate a run turned
     * out to have.
     * It is the reciprocal of main.cpp's tick interval, which is 40 ms
     * and therefore twenty-five of them a second.
     * Every period below is derived from it, so changing the pace is one
     * edit here rather than one per constant.
     */
    inline constexpr std::int32_t kTicksPerSecond = 25;

    /**
     * @brief How much of one resource a building can hold.
     */
    inline constexpr std::int32_t kStockCapacity = 100;

    /**
     * @brief How much of each resource a building is put up with.
     *
     * A tenth of capacity, so a new house needs a delivery fairly soon
     * without starving before a walker could possibly reach it.
     */
    inline constexpr std::int32_t kStockOnCompletion = kStockCapacity / 10;

    /**
     * @brief The most risk a building can carry before it is lost.
     *
     * One ceiling for every risk, so the readouts share a scale and a
     * percentage means the same thing on any line.
     */
    inline constexpr std::int32_t kMaxRisk = 100;

    /**
     * @brief Ticks between one step of the risks and the next.
     *
     * One a second, so a building nobody visits reaches kMaxRisk after
     * a hundred seconds and is lost.
     * One period for every risk rather than one each: the point of a
     * per-building countdown is that two *buildings* stay out of
     * lockstep, and a building's own risks stepping together is no
     * lockstep at all.
     */
    inline constexpr std::int32_t kRiskPeriodTicks = kTicksPerSecond;

    /**
     * @brief Ticks between one serving of stock draining and the next.
     *
     * One every four seconds, so a house put up with kStockOnCompletion
     * has forty seconds to be found by a walker.
     */
    inline constexpr std::int32_t kDrainPeriodTicks = 4 * kTicksPerSecond;

    /**
     * @brief How many occupants share one serving per drain period.
     *
     * **A fuller house eats faster, and an empty one eats nothing.**
     * Each started group of this many people costs one unit of every
     * resource the house holds per period -- so a full tent's five
     * eat two units where a lone settler eats one, and a house whose
     * people have all left stops draining at all.
     */
    inline constexpr std::int32_t kMouthsPerServing = 4;

    /**
     * @brief Ticks between a building being free and sending somebody out.
     *
     * Only counted down once one of its slots is free, since a building
     * may have kMaxWalkersOut out at a time.
     * So it is the pause between one walker and the next rather than a
     * rate anything is emitted at.
     */
    inline constexpr std::int32_t kSpawnPeriodTicks = kTicksPerSecond;

    /**
     * @brief How many walkers one building may have out at once.
     *
     * **Two, because a market sends a buyer and a seller.** The rule
     * used to be one, held in a single handle, and that was exact while
     * every kind that walked sent one walker doing one job. It stops
     * being exact the moment a building has two errands that overlap: a
     * market fetching goods while it is also handing them out, and a
     * workshop hauling its output away while it waits on a delivery.
     * Two is what round one needs and no more; raising it is a change to
     * this number and to the save schema's maxItems, and to nothing
     * else.
     */
    inline constexpr std::size_t kMaxWalkersOut = 2;

    /**
     * @brief Something standing on a cell, and what it is doing there.
     *
     * The cell it stands on is a separate Cell component, the same way a
     * Path's is, so the two can be viewed together.
     *
     * Every countdown here is per building and lives in the building's
     * own component rather than being a modulus on the tick number, for
     * exactly the reason Walker::ticksUntilStep is: two buildings put up
     * a tick apart would otherwise drain, risk and spawn in lockstep for
     * ever, and a replay regenerates each countdown from the same click
     * that created the building.
     */
    struct Building
    {
        BuildingKind kind = BuildingKind::House;

        /**
         * @brief How much of each resource it is holding.
         *
         * Indexed by resourceIndex(), so a building holds an amount per
         * resource without naming any of them.
         * That is what lets a house consume goods without one member
         * per good that would have to be edited to admit another.
         *
         * A building is put up holding a little food and none of the
         * goods somebody has to make first, which is the same reading
         * the version 2 to 3 migration takes of a file written before
         * clay and pottery existed.
         */
        std::array<std::int32_t, kResourceCount> stock{
            kStockOnCompletion};

        /**
         * @brief How close it is to catching fire, out of kMaxRisk.
         *
         * **A risk only ever climbs on its own; a visit is what puts
         * it back.** It used to fall while a coverage countdown said a
         * fireman came recently, which made the danger a function of a
         * clock nobody could see.
         * Now every risk period adds one, unconditionally, and a
         * fireman standing beside the building zeroes it -- so the
         * only safe district is one somebody keeps walking through.
         * At kMaxRisk the building catches fire -- see ignite().
         */
        std::int32_t fireRisk = 0;

        /**
         * @brief How close it is to falling down, out of kMaxRisk.
         *
         * The fire risk's shape exactly, with the engineer as the
         * visitor who zeroes it.
         * At kMaxRisk the building collapses straight to debris --
         * the fire's ending without the fire, see collapse().
         */
        std::int32_t collapseRisk = 0;

        /**
         * @brief How close its people are to sickness, out of kMaxRisk.
         *
         * **The one risk that still answers to a service.** Medicine
         * is a state a doctor's round confers -- Service::Health --
         * so this rises by one per risk period while the medicine has
         * run out and falls by one while there is any, rather than
         * climbing unconditionally as the other two do.
         * Nothing ends a building over it yet: it is a warning a
         * watcher reads off the hover panel.
         */
        std::int32_t diseaseRisk = 0;

        /**
         * @brief Ticks until it may send its next walker out.
         *
         * One short of the period on a fresh building, exactly as
         * WalkerSystem leaves a walker that has just stepped, so one put
         * up now sends its first walker kSpawnPeriodTicks ticks later
         * rather than one tick after that.
         */
        std::int32_t ticksUntilSpawn = kSpawnPeriodTicks - 1;

        /** @brief Ticks until it loses one of each resource it holds. */
        std::int32_t ticksUntilDrain = kDrainPeriodTicks;

        /** @brief Ticks until both risks take their step. */
        std::int32_t ticksUntilRisk = kRiskPeriodTicks;

        /**
         * @brief The walkers it currently has out, one per slot.
         *
         * A building may have kMaxWalkersOut out at a time, and this is
         * how it knows.
         * Holding the handles rather than counting walkers keeps that a
         * lookup rather than a scan of every walker in the world, once
         * per building, once per tick.
         *
         * **A fixed array rather than a vector**, because ecs::Component
         * requires a trivially copyable, standard-layout type and the
         * World copies a component by value on every set().
         *
         * **Every entry is a cache, and world.alive() is the
         * authority.** That is safe because ecs::EntityManager never
         * reuses an index, so a stale handle can only ever be *dead*
         * rather than somebody else, and the only transition an entry
         * makes behind its building's back is alive to dead.
         * kNullEntity needs no case of its own, since alive(kNullEntity)
         * is already false, which is also what an unused slot holds.
         *
         * **The slots are not ordered and mean nothing individually.**
         * A walker goes into the lowest free one, so two buildings that
         * have sent the same walkers in the same order hold them the
         * same way, and nothing may read a slot number as a role.
         */
        std::array<antwika::ecs::Entity, kMaxWalkersOut> walkers{};

        /**
         * @brief Compare two buildings.
         * @param other The building to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

} // namespace antwika::game
