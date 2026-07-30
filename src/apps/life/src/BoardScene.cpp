#include "antwika/life/BoardScene.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/life/BoardLayout.hpp"

namespace antwika::life
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kBackground{.red = 16, .green = 16, .blue = 24};

        constexpr Color kDeadCells{.red = 32, .green = 36, .blue = 48};

        constexpr Color kAliveCell{.red = 96, .green = 224, .blue = 128};
    } // namespace

    void BoardScene::draw(
        IRenderer &renderer, Size canvas, const Board &board) const
    {
        renderer.clear(kBackground);

        // Drawing a cell and hitting one are the same question.
        // So both go through layoutFor().
        const auto layout = layoutFor(canvas, board.width, board.height);

        if (!layout)
        {
            return;
        }

        const auto cell = layout->cell;

        renderer.drawRect(
            Rect{
                .origin = {.x = layout->originX, .y = layout->originY},
                .size = {
                    .width = cell * board.width,
                    .height = cell * board.height}},
            kDeadCells);

        for (std::size_t index = 0; index < board.alive.size(); ++index)
        {
            const bool alive = board.alive[index];
            if (!alive)
            {
                continue;
            }

            const auto column = static_cast<std::uint32_t>(
                index % board.width);
            const auto row = static_cast<std::uint32_t>(index / board.width);

            renderer.drawRect(
                Rect{
                    .origin = {
                        .x = layout->originX +
                             static_cast<std::int32_t>(column * cell),
                        .y = layout->originY +
                             static_cast<std::int32_t>(row * cell)},
                    .size = {.width = cell, .height = cell}},
                kAliveCell);
        }
    }

} // namespace antwika::life
