#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/rules/Health.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    class HealthSystem final : public ecs::ISystem
    {
    public:
        void setFrozen(bool frozen) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        std::vector<ecs::Entity> taken;

        bool frozen = false;
    };

}
