#pragma once

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/GameConfig.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class LabourDispatchSystem final : public ISystem
    {
    public:
        LabourDispatchSystem(
            const PathIndex &paths, GameConfig config) noexcept;

        LabourDispatchSystem(const LabourDispatchSystem &) = delete;
        LabourDispatchSystem(LabourDispatchSystem &&) = delete;

        LabourDispatchSystem &operator=(const LabourDispatchSystem &)
            = delete;
        LabourDispatchSystem &operator=(LabourDispatchSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        const PathIndex &paths;
        GameConfig config;
    };

}
