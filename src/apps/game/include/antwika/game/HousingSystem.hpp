#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/GameConfig.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class HousingSystem final : public ISystem
    {
    public:
        HousingSystem(
            const DesirabilityField &desirability, GameConfig config);

        HousingSystem(const HousingSystem &) = delete;
        HousingSystem(HousingSystem &&) = delete;

        HousingSystem &operator=(const HousingSystem &) = delete;
        HousingSystem &operator=(HousingSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        const DesirabilityField &desirability;
        GameConfig config;
    };

}
