#pragma once

#include <optional>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Get which walker a building sends out.
     * @param kind The kind of building.
     * @return The walker it spawns, or nullopt for a house, which spawns
     * none and only consumes.
     */
    [[nodiscard]] constexpr std::optional<WalkerKind> walkerFor(
        BuildingKind kind) noexcept
    {
        switch (kind)
        {
        case BuildingKind::FoodSource:
            return WalkerKind::Food;
        case BuildingKind::WaterSource:
            return WalkerKind::Water;
        case BuildingKind::FireStation:
            return WalkerKind::Fireman;
        case BuildingKind::ArchitectPost:
            return WalkerKind::Architect;
        case BuildingKind::House:
            break;
        }

        return std::nullopt;
    }

    /**
     * @brief Drains, ages and spawns for every building, once a tick.
     *
     * It runs in a phase of its own, *after* the one the walkers move in,
     * because both write the Building component and only one write per
     * component per phase survives -- see World::commit(). Draining after
     * delivering is also the reading that makes sense: what a walker
     * brought this tick is stock the building then eats from.
     *
     * A building leaves the world the moment it has nothing left, has
     * burnt down or has collapsed.  All three are the same removal: the
     * entity is destroyed and BuildingIndex is told, so the ground is
     * free to build on again on the very next click.
     */
    class BuildingSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over what it spawns onto and
         * records into.
         * @param paths Consulted for somewhere to put a new walker; must
         * outlive this system.
         * @param buildings Cleared as a building is taken down; must
         * outlive this system.
         */
        BuildingSystem(const PathIndex &paths, BuildingIndex &buildings);

        BuildingSystem(const BuildingSystem &) = delete;
        BuildingSystem(BuildingSystem &&) = delete;

        BuildingSystem &operator=(const BuildingSystem &) = delete;
        BuildingSystem &operator=(BuildingSystem &&) = delete;

        /**
         * @brief Age every building one tick.
         * @param world The world to read buildings from and stage changes
         * into.
         * @param tick The tick being processed; unused, because every
         * period is counted per building rather than off the clock.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        [[nodiscard]] bool spawn(World &world, Cell at, WalkerKind kind);

        const PathIndex &paths;
        BuildingIndex &buildings;
    };

} // namespace antwika::game
