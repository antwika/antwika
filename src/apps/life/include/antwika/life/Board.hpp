#pragma once

#include <cstdint>
#include <vector>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    struct Board final
    {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<bool> alive;

        bool operator==(const Board &other) const = default;
    };

    [[nodiscard]] Board readBoard(const World &world, const Grid &grid);

    [[nodiscard]] Board readBoardFromView(
        const World &world, std::uint32_t width, std::uint32_t height);

}
