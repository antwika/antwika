#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

#include <antwika/ecs/World.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Workforce.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief One walker, as a city keeps it while it is put away.
     *
     * The component itself rather than a copy of the fields worth
     * remembering, so a member added to Walker is carried across a city
     * switch without this type hearing about it. That is the opposite
     * choice from SavedWalker, and deliberately: a file has to name
     * every field it holds because a reader of it may be a different
     * build, while this never leaves the process that wrote it.
     */
    struct StoredWalker
    {
        /** @brief Where it stands. */
        Cell at;

        /**
         * @brief What it is doing there.
         *
         * Its `home` is always kNullEntity here: a stored city holds no
         * entity handle at all, since a handle means nothing once the
         * entity it named has been destroyed and remade. Which building
         * sent it is the index below.
         */
        Walker walker;

        /**
         * @brief Which stored building sent it, by index.
         *
         * **An index rather than the ecs::Entity it is in memory**, for
         * SavedWalker's reason: opening a city destroys and recreates
         * every entity on the grid, so the recreated building is a
         * different id from the one that was put away.
         */
        std::optional<std::size_t> home = std::nullopt;

        /**
         * @brief Where it is taking a load, if it is taking one.
         *
         * Its `destination` is always kNullEntity here, for `walker`'s
         * reason; which building it is bound for is the index below.
         * Absent means it roams, which is what a walker with no Errand
         * component does.
         */
        std::optional<Errand> errand = std::nullopt;

        /**
         * @brief Which stored building its errand names, by index.
         *
         * Absent for an errand bound nowhere, which is an ordinary
         * state -- see Errand -- as well as for a walker with no errand
         * at all.
         */
        std::optional<std::size_t> destination = std::nullopt;

        /**
         * @brief Compare two stored walkers.
         * @param other The walker to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const StoredWalker &other) const
            = default;
    };

    /**
     * @brief One building, as a city keeps it while it is put away.
     *
     * Every countdown comes along, exactly as a save carries them: a
     * city reopened with all of them reset is a city whose buildings
     * drain, risk and spawn in lockstep from then on.
     */
    struct StoredBuilding
    {
        /** @brief Where it stands, at the minimum corner of its block. */
        Cell at;

        /**
         * @brief What it is and what it holds.
         *
         * Every entry of its `walkers` is kNullEntity here, for the
         * reason StoredWalker::walker gives; which walkers it has out
         * are the indices below.
         */
        Building building;

        /**
         * @brief Which stored walkers it has out, by index, per slot.
         *
         * One entry per slot rather than a list of the occupied ones,
         * so a building put away and opened again holds each walker in
         * the slot it was in.
         */
        std::array<std::optional<std::size_t>, kMaxWalkersOut> walkers{};

        /**
         * @brief How much longer each service still reaches it.
         *
         * A plain value rather than an optional one, because an absent
         * Coverage component and an all-zero one mean the same thing --
         * see Coverage.hpp. restoreCityGrid() therefore puts a
         * component back only where there is something in it, which is
         * what keeps a city that nobody has ever served from acquiring
         * one on being reopened.
         */
        Coverage coverage{};

        /**
         * @brief How far it is through the batch it is making.
         *
         * Carried across for the reason every countdown here is:
         * a city reopened with all of them reset is a city whose
         * producers finish in lockstep from then on.
         */
        std::optional<Production> production = std::nullopt;

        /**
         * @brief Who lives there and how close it is to a change.
         *
         * Carried across for the reason every countdown here is: a city
         * reopened with its housing countdowns reset is a city whose
         * houses grow and shrink in lockstep from then on -- and one
         * reopened with its *levels* reset is a district somebody spent
         * a run building up and lost by looking at the world map.
         *
         * Optional rather than a plain value, unlike the coverage above
         * it: an absent Household and a default one do mean the same
         * thing, but a house on the bottom level with fresh countdowns
         * is a state HousingSystem writes back, so keeping the optional
         * is what makes a reopened city hold exactly the components the
         * closed one held.
         */
        std::optional<Household> household = std::nullopt;

        /**
         * @brief How many of the city's people were working there.
         *
         * Carried across for the household's reason: a city reopened
         * with its workplaces unstaffed is a city whose walkers all stop
         * for a tick, and a run that switched cities would then differ
         * from one that did not.
         *
         * Optional rather than a plain value, exactly as the household
         * above it is: an absent Workforce means fully staffed rather
         * than empty -- see LabourQuery.hpp -- so putting one back where
         * there was none would change what a reopened city does.
         */
        std::optional<Workforce> workforce = std::nullopt;

        /**
         * @brief Compare two stored buildings.
         * @param other The building to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const StoredBuilding &other) const
            = default;
    };

    /**
     * @brief Everything standing on one city's grid, as a value.
     *
     * A city that is not the live one has no entities of its own, since
     * there is one World and the entities in it are the live city's.
     * This is what a closed city keeps instead, and it is a plain
     * comparable value so "these two cities hold the same things" is one
     * EXPECT_EQ.
     *
     * The roads are **not** here: they are the PathIndex the city keeps
     * anyway, and a second list of them would be a second truth to keep
     * in step. restoreCityGrid() lays the path entities back down from
     * that index instead.
     */
    struct CityGrid
    {
        /** @brief Every walker, in the world's own order. */
        std::vector<StoredWalker> walkers;

        /** @brief Every building, in the world's own order. */
        std::vector<StoredBuilding> buildings;

        /**
         * @brief Compare two city grids.
         * @param other The grid to compare against.
         * @return True when both hold the same things in the same order.
         */
        [[nodiscard]] bool operator==(const CityGrid &other) const
            = default;
    };

    /**
     * @brief Take everything standing on the live grid as a value.
     *
     * Read as of the last commit(), like every other read of a World, so
     * a caller with staged writes it wants included commits first -- see
     * WorldMapState::closeCity, which is the one caller that has any.
     *
     * The building/walker link is written as a pair of indices into the
     * two arrays this produces, exactly as saveGameOf() writes it, so
     * nothing here depends on how one run happened to number its
     * entities.
     *
     * @param world Read for the walkers and the buildings.
     * @return What is standing there.
     */
    [[nodiscard]] CityGrid cityGridOf(const World &world);

    /**
     * @brief Put a city's contents onto the live grid.
     *
     * Everything already standing there is destroyed rather than added
     * to, since opening a city is showing that city and not merging two.
     * Both the destruction and the creation are *staged*, the way every
     * other write to a World is, so they land at the next commit()
     * together -- which is what keeps a switch that happens part-way
     * through a tick from being half-visible to whatever runs after it.
     *
     * Every entity is created before any component is added, because
     * create() is immediate where add() is staged: a link therefore has
     * to be built into the component rather than written onto it
     * afterwards.
     *
     * @param world Where the entities are destroyed and recreated.
     * @param built Rebuilt from the buildings this puts down, so it
     * cannot describe a city other than the live one.
     * @param paths The roads of the city being opened; one path entity
     * is laid per cell, so the World and the index agree.
     * @param grid What to put down.
     */
    void restoreCityGrid(
        World &world,
        BuildingIndex &built,
        const PathIndex &paths,
        const CityGrid &grid);

} // namespace antwika::game
