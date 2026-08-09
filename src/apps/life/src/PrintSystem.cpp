#include "antwika/life/PrintSystem.hpp"

#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    PrintSystem::PrintSystem(std::uint32_t width, std::ostream &out)
        : width(width), out(out)
    {
    }

    void PrintSystem::update(World &world, antwika::time::Tick tick)
    {
        out << "After tick " << tick << ":\n";

        std::uint32_t column = 0;
        for (const auto entity : world.view<Cell>())
        {
            out << (world.get<Cell>(entity).alive ? '#' : '.');
            if (++column == width)
            {
                out << '\n';
                column = 0;
            }
        }
    }

}
