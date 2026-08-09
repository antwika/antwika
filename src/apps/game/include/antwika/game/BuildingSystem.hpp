#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class BuildingSystem final : public ISystem
    {
    public:
        BuildingSystem(
            BuildingIndex &built, GridExtent extent, GameConfig config);

        BuildingSystem(const BuildingSystem &) = delete;
        BuildingSystem(BuildingSystem &&) = delete;

        BuildingSystem &operator=(const BuildingSystem &) = delete;
        BuildingSystem &operator=(BuildingSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        BuildingIndex &built;
        GridExtent extent;
        GameConfig config;
    };

}
