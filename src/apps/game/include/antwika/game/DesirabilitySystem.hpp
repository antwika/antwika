#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Desirability.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class DesirabilitySystem final : public ISystem
    {
    public:
        DesirabilitySystem(
            DesirabilityField &field, GridExtent extent) noexcept;

        DesirabilitySystem(const DesirabilitySystem &) = delete;
        DesirabilitySystem(DesirabilitySystem &&) = delete;

        DesirabilitySystem &operator=(const DesirabilitySystem &) = delete;
        DesirabilitySystem &operator=(DesirabilitySystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        DesirabilityField &field;
        GridExtent extent;
    };

}
