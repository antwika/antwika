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

#include "antwika/game/FpsReadout.hpp"

using antwika::game::describeFps;
using antwika::gfx::Size;
using antwika::gfx::textSize;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;

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
} // namespace

TEST(FpsReadoutTest, DescribeFps_ShowsTheRateItIsGiven)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, 60)),
        ::testing::Contains(std::string{"fps 60"}));
}

// The first second of a run has measured nothing.
// So it says so, rather than claiming the machine drew no frames.
TEST(FpsReadoutTest, DescribeFps_ShowsAPlaceholderUntilThereIsARate)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, std::nullopt)),
        ::testing::Contains(std::string{"fps --"}));
}

// The two the placeholder exists to keep apart.
// A stalled machine really is drawing zero frames a second.
TEST(FpsReadoutTest, DescribeFps_TellsNoRateApartFromNoFrames)
{
    EXPECT_THAT(
        textsOf(describeFps(kCanvas, 0)),
        ::testing::Contains(std::string{"fps 0"}));
    EXPECT_NE(describeFps(kCanvas, std::nullopt), describeFps(kCanvas, 0));
}

// The picture is a value, so the same arguments give the same picture.
TEST(FpsReadoutTest, DescribeFps_IsAPureFunctionOfItsArguments)
{
    EXPECT_EQ(describeFps(kCanvas, 42), describeFps(kCanvas, 42));
    EXPECT_NE(describeFps(kCanvas, 42), describeFps(kCanvas, 43));
}

// The toolbar owns the left corner, so this has to be in the right one.
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

// Nothing here can clip, so the layout has to do the containing.
TEST(FpsReadoutTest, DescribeFps_KeepsEverythingInsideTheCanvas)
{
    const auto right = static_cast<std::int32_t>(kCanvas.width);
    const auto bottom = static_cast<std::int32_t>(kCanvas.height);

    for (const auto &command : describeFps(kCanvas, 1000))
    {
        if (const auto *fill = std::get_if<FillRect>(&command))
        {
            EXPECT_GE(fill->rect.origin.x, 0);
            EXPECT_GE(fill->rect.origin.y, 0);
            EXPECT_LE(
                fill->rect.origin.x
                    + static_cast<std::int32_t>(fill->rect.size.width),
                right);
            EXPECT_LE(
                fill->rect.origin.y
                    + static_cast<std::int32_t>(fill->rect.size.height),
                bottom);

            continue;
        }

        const auto &text = std::get<DrawText>(command);
        const auto extent = textSize(text.text, text.scale);

        EXPECT_GE(text.origin.x, 0);
        EXPECT_GE(text.origin.y, 0);
        EXPECT_LE(
            text.origin.x + static_cast<std::int32_t>(extent.width), right);
        EXPECT_LE(
            text.origin.y + static_cast<std::int32_t>(extent.height),
            bottom);
    }
}
