#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/intent/DirectionKeys.hpp>
#include <antwika/rules/Orientation.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::system
{

    class OrientationSystem final : public ecs::ISystem
    {
    public:
        explicit OrientationSystem(
            const intent::DirectionKeys &lookKeys) noexcept;

        void update(ecs::World &world, time::Tick tick) override;

    private:
        const intent::DirectionKeys *lookKeys;
    };

}
