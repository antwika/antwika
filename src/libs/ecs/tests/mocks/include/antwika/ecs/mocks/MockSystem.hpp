#pragma once

#include <gmock/gmock.h>

#include <antwika/ecs/ISystem.hpp>

namespace antwika::ecs::mocks
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class MockSystem : public ISystem
    {
    public:
        MOCK_METHOD(
            void,
            update,
            (World & world, antwika::time::Tick tick),
            (override));
    };

}
