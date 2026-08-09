#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameConfig.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class StaffingSystem final : public ISystem
    {
    public:
        explicit StaffingSystem(GameConfig config);

        StaffingSystem(const StaffingSystem &) = delete;
        StaffingSystem(StaffingSystem &&) = delete;

        StaffingSystem &operator=(const StaffingSystem &) = delete;
        StaffingSystem &operator=(StaffingSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        GameConfig config;
    };

}
