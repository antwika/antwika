#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/GameConfig.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/Ruin.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class RuinSystem final : public ISystem
    {
    public:
        RuinSystem(
            BuildingIndex &built,
            GridExtent extent,
            GameConfig config) noexcept;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        void spread(World &world, Cell at, const Ruin &ruin);

        BuildingIndex &built;
        GridExtent extent;
        GameConfig config;
    };

}
