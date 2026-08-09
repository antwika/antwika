#pragma once

#include <functional>
#include <utility>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::ecs_commons
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class GatedSystem final : public ISystem
    {
    public:
        using Allows = std::function<bool()>;

        GatedSystem(ISystem &inner, Allows allows)
            : inner(inner), allows(std::move(allows))
        {
        }

        GatedSystem(const GatedSystem &) = delete;
        GatedSystem(GatedSystem &&) = delete;

        GatedSystem &operator=(const GatedSystem &) = delete;
        GatedSystem &operator=(GatedSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override
        {
            if (!allows())
            {
                return;
            }

            inner.update(world, tick);
        }

    private:
        ISystem &inner;
        Allows allows;
    };

}
