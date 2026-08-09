#pragma once

#include <cstddef>
#include <optional>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    [[nodiscard]] std::optional<Cell> spawnCellFor(
        Cell origin, Footprint footprint, const PathIndex &paths);

    [[nodiscard]] std::optional<std::size_t> freeWalkerSlot(
        const World &world, const Building &building);

    [[nodiscard]] bool hasWalkerOfKind(
        const World &world, const Building &building, WalkerKind kind);

    class SpawnSystem final : public ISystem
    {
    public:
        SpawnSystem(const PathIndex &paths, GameConfig config);

        SpawnSystem(const SpawnSystem &) = delete;
        SpawnSystem(SpawnSystem &&) = delete;

        SpawnSystem &operator=(const SpawnSystem &) = delete;
        SpawnSystem &operator=(SpawnSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GameConfig config;
    };

}
