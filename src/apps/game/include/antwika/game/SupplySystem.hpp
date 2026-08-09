#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class SupplySystem final : public ISystem
    {
    public:
        SupplySystem(
            const PathIndex &paths, GridExtent extent, GameConfig config);

        SupplySystem(const SupplySystem &) = delete;
        SupplySystem(SupplySystem &&) = delete;

        SupplySystem &operator=(const SupplySystem &) = delete;
        SupplySystem &operator=(SupplySystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GridExtent extent;
        GameConfig config;
    };

}
