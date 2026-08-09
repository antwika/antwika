#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs_commons/GatedSystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/DragState.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class DragPausedSystem final : public ISystem
    {
    public:
        DragPausedSystem(ISystem &inner, const DragState &drag);

        DragPausedSystem(const DragPausedSystem &) = delete;
        DragPausedSystem(DragPausedSystem &&) = delete;

        DragPausedSystem &operator=(const DragPausedSystem &) = delete;
        DragPausedSystem &operator=(DragPausedSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        antwika::ecs_commons::GatedSystem gate;
    };

}
