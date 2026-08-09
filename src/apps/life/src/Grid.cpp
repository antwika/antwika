#include "antwika/life/Grid.hpp"

#include "antwika/life/Cell.hpp"

namespace antwika::life
{

    Grid::Grid(World &world, std::uint32_t width, std::uint32_t height)
        : widthValue(width), heightValue(height)
    {
        entities.reserve(static_cast<std::size_t>(width) * height);
        for (std::uint32_t y = 0; y < height; ++y)
        {
            for (std::uint32_t x = 0; x < width; ++x)
            {
                const auto entity = world.create();
                world.add<Cell>(entity, Cell{.alive = false});
                entities.push_back(entity);
            }
        }
    }

    std::uint32_t Grid::width() const noexcept
    {
        return widthValue;
    }

    std::uint32_t Grid::height() const noexcept
    {
        return heightValue;
    }

    bool Grid::contains(std::uint32_t x, std::uint32_t y) const noexcept
    {
        return x < widthValue && y < heightValue;
    }

    Entity Grid::entityAt(std::uint32_t x, std::uint32_t y) const
    {
        return entities.at(static_cast<std::size_t>(y) * widthValue + x);
    }

}
