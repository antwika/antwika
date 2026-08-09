#pragma once

#include <cstdint>
#include <ostream>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    class PrintSystem final : public ISystem
    {
    public:
        PrintSystem(std::uint32_t width, std::ostream &out);

        PrintSystem(const PrintSystem &) = delete;
        PrintSystem(PrintSystem &&) = delete;

        PrintSystem &operator=(const PrintSystem &) = delete;
        PrintSystem &operator=(PrintSystem &&) = delete;

        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::uint32_t width;
        std::ostream &out;
    };

}
