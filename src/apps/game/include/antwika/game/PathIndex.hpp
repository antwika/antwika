#pragma once

#include <cstddef>
#include <set>

#include "antwika/game/Cell.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    class PathIndex final
    {
    public:
        bool insert(Cell cell);

        bool erase(Cell cell);

        [[nodiscard]] bool has(Cell cell) const;

        [[nodiscard]] Neighbours neighboursOf(Cell cell) const;

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] const std::set<Cell> &cells() const noexcept;

    private:
        std::set<Cell> paths;
    };

}
