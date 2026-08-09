#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/ecs/EcsError.hpp"
#include "antwika/ecs/ISystem.hpp"
#include "antwika/ecs/Phase.hpp"
#include "antwika/ecs/World.hpp"

namespace antwika::ecs
{

    class SystemScheduler final
    {
    public:
        [[nodiscard]] PhaseId createPhase(std::string_view name);

        void addSystem(PhaseId phase, ISystem &system);

        void run(World &world, antwika::time::Tick tick);

    private:
        struct Phase final
        {
            std::string name;
            std::vector<ISystem *> systems;
        };

        std::vector<Phase> phases;
    };

}
