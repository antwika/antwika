#include "antwika/life/BoardScene.hpp"

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

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

        if (board.width == 0 || board.height == 0)
        {
            return;
        }

        const auto byWidth = canvas.width / board.width;
        const auto byHeight = canvas.height / board.height;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        // Cells are never rounded up to a minimum size.
        // The smaller quotient keeps the board inside the canvas.
        // So the centring offsets below cannot underflow.
        if (cell == 0)
        {
            return;
        }

        const auto used = Size{
            .width = cell * board.width, .height = cell * board.height};
        const auto originX =
            static_cast<std::int32_t>((canvas.width - used.width) / 2);
        const auto originY =
            static_cast<std::int32_t>((canvas.height - used.height) / 2);

        renderer.drawRect(
            Rect{.origin = {.x = originX, .y = originY}, .size = used},
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
                        .x = originX +
                             static_cast<std::int32_t>(column * cell),
                        .y = originY + static_cast<std::int32_t>(row * cell)},
                    .size = {.width = cell, .height = cell}},
                kAliveCell);
        }
    }

} // namespace antwika::life
