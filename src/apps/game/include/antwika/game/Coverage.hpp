#pragma once

#include <array>
#include <cstdint>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Service.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief How long one visit's worth of a service lasts.
     *
     * Twenty seconds of it, which is comfortably longer than a walker's
     * round trip and comfortably shorter than a run: a district stays
     * served while walkers keep reaching it and goes dark within sight
     * of the road being cut.
     *
     * **Counted in ticks left rather than in a percentage**, because the
     * decay is then a subtraction of one per tick and needs no period,
     * no phase and no second countdown to keep two buildings out of
     * lockstep -- which is the trap every other countdown in this
     * application carries a field to avoid.
     */
    inline constexpr std::int32_t kCoverageFull = 20 * kTicksPerSecond;

    /**
     * @brief How much longer each service still reaches a building.
     *
     * **A component of its own rather than four more fields on
     * Building**, and the reason is the rule the whole increment is
     * written under: every new component is optional, and its absence is
     * the value the game had before the component existed. A building
     * put up a moment ago has none of this, and coverageOf() answers
     * zero for it -- which is exactly what a save written before
     * coverage existed means, so the format grows by an optional member
     * and no migration.
     *
     * Indexed by serviceIndex(), so a building carries one countdown per
     * service without naming any of them, exactly as Building::stock
     * carries one amount per resource.
     */
    struct Coverage
    {
        /** @brief Ticks of each service still reaching this building. */
        std::array<std::int32_t, kServiceCount> ticksLeft{};

        /**
         * @brief Compare two coverages.
         * @param other The coverage to compare against.
         * @return True when every service's countdown matches.
         */
        [[nodiscard]] bool operator==(const Coverage &other) const = default;
    };

    /**
     * @brief Get everything still reaching a building.
     *
     * **Total, and an entity with no Coverage answers zero rather than
     * throwing.** That is what lets a consumer -- a housing rule, a
     * rating, a hover panel -- be written against this before anything
     * has ever conferred a service, and what makes an absent component
     * mean "uncovered" rather than "unknown".
     *
     * @param world Read as of its last commit(), like every other read.
     * @param entity The building to ask about; it need not be alive.
     * @return Its coverage, or an all-zero one when it has none.
     */
    [[nodiscard]] Coverage coverageOf(
        const World &world, antwika::ecs::Entity entity);

    /**
     * @brief Get how much longer one service still reaches a building.
     * @param world Read as of its last commit().
     * @param entity The building to ask about; it need not be alive.
     * @param service The service to ask about.
     * @return Ticks left of it, between zero and kCoverageFull.
     */
    [[nodiscard]] std::int32_t coverageOf(
        const World &world,
        antwika::ecs::Entity entity,
        Service service);

    /**
     * @brief Write a building's coverage, whether or not it had any.
     *
     * **The one place a Coverage is written, and it exists because
     * "add or set" is a decision rather than a detail.** World::add()
     * is staged and World::set() is immediate and refuses an entity
     * that has no such component yet, so every writer would otherwise
     * have to ask which of the two it wants; asking once here is what
     * keeps a building from acquiring an all-zero component just to
     * make one call site simpler.
     *
     * @param world Staged into; the write lands at the next commit().
     * @param entity The building to write; must be alive.
     * @param coverage What it is to hold.
     */
    void setCoverage(
        World &world, antwika::ecs::Entity entity, Coverage coverage);

} // namespace antwika::game
