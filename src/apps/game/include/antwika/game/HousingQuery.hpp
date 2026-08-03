#pragma once

#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/HousingLevel.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @file
     * @brief The read-only face of housing, for everything that is not
     * HousingSystem.
     *
     * **Published as a seam rather than left as three field reads**, so a
     * rating, a renderer or a population rule asks a function about an
     * entity instead of asking whether a component is there and then
     * reaching into it. Every answer here is total: an entity with no
     * Household -- a building put up this tick, a well, a road, a handle
     * whose entity is long dead -- answers the value the game had before
     * housing existed rather than throwing.
     */

    /**
     * @brief Get how well a building's household lives.
     *
     * **A building with no Household answers the bottom level rather
     * than throwing, and that includes a building nobody lives in.** A
     * caller wanting to know whether anybody lives there at all asks
     * housesPeople() about its kind; this answers the tier, and the
     * bottom tier is what a house that has never grown is on.
     *
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @return Its level, or HousingLevel::Tent when it has no household.
     */
    [[nodiscard]] HousingLevel levelOf(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Get how many people a level houses.
     *
     * **Named for the population rather than called capacityOf()**,
     * which the plan for this increment suggested: Store.hpp already
     * answers capacityOf(BuildingKind) about how much of a good a
     * building holds, and two overloads of one name meaning two
     * different capacities is the kind of ambiguity a reader has to
     * resolve by looking at the argument's type.
     *
     * @param level The level to ask about.
     * @return How many people live there when it is full.
     */
    [[nodiscard]] constexpr std::int32_t populationCapacityOf(
        HousingLevel level) noexcept
    {
        return requirementOf(level).populationCapacity;
    }

    /**
     * @brief Get how many people live in a building right now.
     *
     * Zero for a building with no household, which in this increment is
     * every building in every city -- W3 stores the number and the rules
     * that move it are a later workstream's.
     *
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @return How many people live there.
     */
    [[nodiscard]] std::int32_t populationAt(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Get how much of one good a standing building can hold.
     *
     * **The one crossing between a kind's capacity and a tier's.** A
     * house's shelf grows with its level -- stockCapacityOf() -- and
     * everything else holds what Store.hpp's capacityOf() says, so a
     * caller with an entity in hand asks here and never chooses
     * between the two tables itself.
     *
     * Total on levelOf()'s terms: a building with no household is on
     * the bottom tier, which for every kind nobody lives in answers
     * capacityOf(kind) exactly as before.
     *
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @param kind Its kind, which the caller already has.
     * @return How much of one good its shelves hold, per resource.
     */
    [[nodiscard]] std::int32_t stockCapacityAt(
        const World &world,
        antwika::ecs::Entity entity,
        BuildingKind kind);

    static_assert(
        populationCapacityOf(HousingLevel::Tent)
        < populationCapacityOf(HousingLevel::Cottage));

} // namespace antwika::game
