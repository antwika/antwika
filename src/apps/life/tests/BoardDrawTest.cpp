#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/BoardScene.hpp"

namespace
{
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::life::Board;
    using antwika::life::BoardScene;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

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

TEST(BoardDrawTest, Draw_DrawsABoardOfLife)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const BoardScene scene;

    scene.draw(renderer, kCanvas, seeded());
}
