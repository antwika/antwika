#pragma once

#include <cstddef>
#include <set>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"

namespace antwika::game
{

    class BuildingIndex final
    {
    public:
        bool insert(Cell origin, Footprint footprint);

        bool erase(Cell origin, Footprint footprint);

        [[nodiscard]] bool has(Cell cell) const;

        [[nodiscard]] bool free(Cell origin, Footprint footprint) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const std::set<Cell> &cells() const noexcept;

    private:
        std::set<Cell> occupied;
    };

}
