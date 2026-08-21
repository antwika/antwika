#pragma once

#include <vector>
#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::gameplay::fakes
{

    class FakeMarkingSystem final : public ecs::ISystem
    {
    public:
        FakeMarkingSystem(std::vector<int> &marks, const int mark)
            : marks(&marks), mark(mark)
        {
        }

        void update(ecs::World &, time::Tick) override
        {
            marks->push_back(mark);
        }

    private:
        std::vector<int> *marks;
        int mark;
    };

}
