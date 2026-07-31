#include "antwika/tower_defence/ScoreOverlay.hpp"

#include <utility>

namespace antwika::tower_defence
{

    ScoreOverlay::ScoreOverlay(const Size canvas) : area(canvas)
    {
    }

    Size ScoreOverlay::canvas() const noexcept
    {
        return area;
    }

    void ScoreOverlay::set(DrawList commands)
    {
        picture = std::move(commands);
    }

    const DrawList &ScoreOverlay::commands() const noexcept
    {
        return picture;
    }

} // namespace antwika::tower_defence
