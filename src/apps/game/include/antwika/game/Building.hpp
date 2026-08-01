#pragma once

#include <array>
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
     * @brief The most risk a building can carry before it is gone.
     */
    inline constexpr std::int32_t kMaxRisk = 100;

    /**
     * @brief How much risk a passing fireman or architect takes off.
     */
    inline constexpr std::int32_t kRiskRelief = 25;

    /**
     * @brief Ticks between one point of risk and the next.
     *
     * One a second, so a building nobody tends reaches kMaxRisk after a
     * hundred seconds and is gone.
     */
    inline constexpr std::int32_t kRiskPeriodTicks = kTicksPerSecond;

    /**
     * @brief Ticks between one unit of stock draining and the next.
     *
     * One every four seconds, so a house put up with kStockOnCompletion
     * has forty seconds to be found by a walker.
     */
    inline constexpr std::int32_t kDrainPeriodTicks = 4 * kTicksPerSecond;

    /**
     * @brief Ticks between a building being free and sending somebody out.
     *
     * Only counted down once its last walker is gone, since a building
     * may have one out at a time.
     * So it is the pause between one walker and the next rather than a
     * rate anything is emitted at.
     */
    inline constexpr std::int32_t kSpawnPeriodTicks = kTicksPerSecond;

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
         * resource without naming either of them.
         * That is what lets a house consume two goods without two
         * members that would have to be edited to admit a third.
         */
        std::array<std::int32_t, kResourceCount> stock{
            kStockOnCompletion, kStockOnCompletion};

        /**
         * @brief How close it is to being lost, out of kMaxRisk.
         */
        std::int32_t risk = 0;

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

        /** @brief Ticks until it gains a point of risk. */
        std::int32_t ticksUntilRisk = kRiskPeriodTicks;

        /**
         * @brief The walker it currently has out, if it has one.
         *
         * A building may have one walker out at a time, and this is how
         * it knows.
         * Holding the handle rather than counting walkers keeps that a
         * lookup rather than a scan of every walker in the world, once
         * per building, once per tick.
         *
         * **It is a cache, and world.alive() is the authority.**
         * That is safe because an ecs::Entity carries a generation as
         * well as an index: a reused index comes back with the generation
         * bumped, so a handle from before the reuse reads as dead rather
         * than as its successor.
         * A stale handle can therefore only ever be *dead* rather than
         * somebody else, and the only transition this value makes behind
         * its building's back is alive to dead.
         * kNullEntity needs no case of its own, since alive(kNullEntity)
         * is already false.
         */
        antwika::ecs::Entity walker = antwika::ecs::kNullEntity;

        /**
         * @brief Compare two buildings.
         * @param other The building to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const Building &other) const = default;
    };

} // namespace antwika::game
