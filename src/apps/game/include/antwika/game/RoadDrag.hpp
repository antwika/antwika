#pragma once

#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    class RoadDrag final
    {
    public:
        void begin(Cell cell) noexcept;

        void dragTo(Cell cell) noexcept;

        void finish() noexcept;

        [[nodiscard]] bool active() const noexcept;

        [[nodiscard]] Cell start() const noexcept;

        [[nodiscard]] Cell end() const noexcept;

    private:
        bool dragging = false;
        Cell from{};
        Cell to{};
    };

}
