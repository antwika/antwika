#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardScene.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::life::Board;
    using antwika::life::BoardScene;

    constexpr std::uint32_t kAcross = 48;

    constexpr std::uint32_t kDown = 32;

    constexpr Size kCanvas{
        .width = kAcross * 10, .height = kDown * 10};

    [[nodiscard]] Board seeded()
    {
        Board board{
            .width = kAcross,
            .height = kDown,
            .alive = std::vector<bool>(kAcross * kDown, false)};

        std::uint32_t step = 7;

        for (std::uint32_t cell = 0; cell < board.alive.size(); ++cell)
        {
            step = (step * 31 + 17) % 101;
            board.alive[cell] = step < 38;
        }

        return board;
    }
}

TEST(BoardPreviewTest, Draw_WritesABoardOfLife)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "life",
             .title = "Antwika Life",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const BoardScene scene;
                scene.draw(renderer, kCanvas, seeded());
            })
            .empty());
}
