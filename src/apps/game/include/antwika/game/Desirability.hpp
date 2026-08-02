#pragma once

#include <array>
#include <cstdint>
#include <map>

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief How much one kind of building changes how nice it is to
     * live nearby, and how far that reaches.
     *
     * Integers, like everything else a replay has to reproduce: a
     * falloff expressed as a fraction would be the first floating-point
     * number in the tick path, and two toolchains do not have to agree
     * on where it rounds.
     */
    struct DesirabilitySource
    {
        /** @brief What it is worth on the cells it stands on. */
        std::int32_t contribution = 0;

        /**
         * @brief How many cells away it still reaches.
         *
         * Chebyshev distance from the block, so the reach is a square
         * around it -- which under this projection is a diamond on
         * screen, the same shape as everything else here.
         * Zero means it changes nothing at all and is skipped.
         */
        std::int32_t radius = 0;

        /**
         * @brief Compare two sources.
         * @param other The source to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] constexpr bool operator==(
            const DesirabilitySource &other) const = default;
    };

    /**
     * @brief What each kind of building does to the ground around it.
     *
     * A table keyed by kind for footprintOf()'s reason: the value must
     * not be able to disagree with the kind that placed it, and a ghost
     * has to be able to say what a building would be worth before any
     * entity exists.
     *
     * A house is worth nothing to its neighbours on purpose. Housing
     * that raised its own desirability would be a feedback loop with no
     * input, and the whole point of the field is that a district is made
     * pleasant by what a player *chooses* to put next to it.
     */
    inline constexpr std::array<DesirabilitySource, kBuildingKindCount>
        kDesirabilityOf{{
            {.contribution = 0, .radius = 0},   // House
            {.contribution = -1, .radius = 3},  // Farm
            {.contribution = -3, .radius = 4},  // ClayPit
            {.contribution = -2, .radius = 4},  // Workshop
            {.contribution = -2, .radius = 4},  // Storage
            {.contribution = 2, .radius = 5},   // Market
            {.contribution = 1, .radius = 4},   // Well
            {.contribution = 2, .radius = 4},   // Doctor
            {.contribution = 1, .radius = 3},   // FireStation
            {.contribution = 1, .radius = 3},   // EngineerPost
        }};

    /**
     * @brief Get what one kind of building is worth to its surroundings.
     * @param kind The kind to ask about.
     * @return Its contribution and reach.
     */
    [[nodiscard]] constexpr DesirabilitySource desirabilityOf(
        BuildingKind kind) noexcept
    {
        return kDesirabilityOf[
            buildingKindIndex(kind) % kBuildingKindCount];
    }

    /**
     * @brief How nice it is to live on each cell that anything reaches.
     *
     * **A cell with no entry and a cell entered as zero would mean the
     * same thing, so only the first exists.** desirabilityOf() drops
     * every zero it works out, which is what makes two fields comparable
     * with EXPECT_EQ rather than only cell by cell.
     *
     * A std::map rather than a grid-shaped vector, because the reach of
     * a building is sparse against the whole extent and because a map is
     * already ordered by Cell -- so anything that walks the field walks
     * it in a total order somebody can name.
     */
    using DesirabilityField = std::map<Cell, std::int32_t>;

    /**
     * @brief Work out what one source is worth at a distance from it.
     *
     * A linear integer falloff: the whole contribution on the block
     * itself, nothing at the radius, and the straight line between.
     * Truncation toward zero is what integer division does and is
     * deliberate -- it is the same on every toolchain, which a rounding
     * rule of our own would have to be argued for.
     *
     * @param source What the building is worth and how far it reaches.
     * @param distance Chebyshev cells from the block, below the radius.
     * @return The contribution at that distance.
     */
    [[nodiscard]] std::int32_t desirabilityFrom(
        DesirabilitySource source, std::int32_t distance) noexcept;

    /**
     * @brief Build the whole field from what is standing.
     *
     * **A sum of integer contributions, and that is the whole
     * determinism argument.** Addition is commutative and associative,
     * so the field is a pure function of the *set* of buildings rather
     * than of the order a view happened to walk them in -- which is what
     * lets this read ecs::View directly, whose order is "whichever
     * storage has the fewest entities" and is nobody's to name.
     *
     * @param world Read for the buildings, as of its last commit().
     * @param extent The bounds to keep the field inside; a contribution
     * reaching off the grid falls off it, because a cell nothing can
     * ever be built on has no desirability to speak of.
     * @return One entry per cell with a non-zero value.
     */
    [[nodiscard]] DesirabilityField desirabilityFieldOf(
        const World &world, GridExtent extent);

    /**
     * @brief Read the field at one cell.
     *
     * Total, exactly as coverageOf() is: a cell nothing reaches answers
     * zero rather than being absent, which is what lets a consumer be
     * written without a lookup of its own.
     *
     * @param field The field to read.
     * @param cell The cell to ask about.
     * @return Its desirability, or zero where nothing reaches it.
     */
    [[nodiscard]] std::int32_t desirabilityAt(
        const DesirabilityField &field, Cell cell) noexcept;

    // A radius of zero is a building that changes nothing.
    // Anything else would divide by it -- see desirabilityFrom().
    static_assert(desirabilityOf(BuildingKind::House).radius == 0);
    static_assert(desirabilityOf(BuildingKind::House).contribution == 0);

} // namespace antwika::game
