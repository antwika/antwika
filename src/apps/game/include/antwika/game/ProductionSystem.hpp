#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameConfig.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class ProductionSystem final : public ISystem
    {
    public:
        explicit ProductionSystem(GameConfig config);

        ProductionSystem(const ProductionSystem &) = delete;
        ProductionSystem(ProductionSystem &&) = delete;

        ProductionSystem &operator=(const ProductionSystem &) = delete;
        ProductionSystem &operator=(ProductionSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        GameConfig config;
    };

}
