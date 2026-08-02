#include "antwika/sudoku/BoardOverlay.hpp"

#include <utility>

namespace antwika::sudoku
{

    BoardOverlay::BoardOverlay(const Size canvas) : area(canvas)
    {
    }

    Size BoardOverlay::canvas() const noexcept
    {
        return area;
    }

    void BoardOverlay::set(DrawList commands)
    {
        picture = std::move(commands);
    }

    const DrawList &BoardOverlay::commands() const noexcept
    {
        return picture;
    }

} // namespace antwika::sudoku
