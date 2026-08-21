#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/rules/Orientation.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    class OrientationSystem final : public ecs::ISystem
    {
    public:
        explicit OrientationSystem(
            const input::DirectionKeys &lookKeys) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const input::DirectionKeys *lookKeys;
    };

}
