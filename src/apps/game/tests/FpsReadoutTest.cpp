#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/support/DrawBounds.hpp>

#include "antwika/game/FpsReadout.hpp"

using antwika::game::describeFps;
using antwika::gfx::Size;
using antwika::gfx::textSize;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::support::expectInsideCanvas;

namespace
{
    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] std::vector<std::string> textsOf(const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }
}

TEST(FpsReadoutTest, DescribeFps_ShowsTheRateItIsGiven)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, 60)),
        ::testing::Contains(std::string{"fps 60"}));
}

TEST(FpsReadoutTest, DescribeFps_ShowsAPlaceholderUntilThereIsARate)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, std::nullopt)),
        ::testing::Contains(std::string{"fps --"}));
}

TEST(FpsReadoutTest, DescribeFps_TellsNoRateApartFromNoFrames)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, 0)),
        ::testing::Contains(std::string{"fps 0"}));
    EXPECT_NE(describeFps(kCanvas, std::nullopt), describeFps(kCanvas, 0));
}

TEST(FpsReadoutTest, DescribeFps_SaysADifferentRateDifferently)
{
    EXPECT_NE(describeFps(kCanvas, 42), describeFps(kCanvas, 43));
}

TEST(FpsReadoutTest, DescribeFps_SitsInTheOppositeCornerToTheToolbar)
{
    const auto commands = describeFps(kCanvas, 60);
    const auto middle = static_cast<std::int32_t>(kCanvas.width) / 2;

    bool drewSomething = false;

    for (const auto &command : commands)
    {
        if (const auto *text = std::get_if<DrawText>(&command))
        {
            EXPECT_GT(text->origin.x, middle);
            drewSomething = true;
        }
    }

    EXPECT_TRUE(drewSomething);
}

TEST(FpsReadoutTest, DescribeFps_KeepsEverythingInsideTheCanvas)
{
    expectInsideCanvas(describeFps(kCanvas, 1000), kCanvas);
}
