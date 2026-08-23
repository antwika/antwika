#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/editor/ui/IconSheet.hpp"
#include "antwika/editor/ui/IconsView.hpp"

using antwika::editor::IconsView;
using antwika::editor::getIconPixelColor;
using antwika::editor::kIconCellSize;
using antwika::gfx::Bitmap;
using antwika::gfx::ITexture;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using ::testing::NiceMock;

namespace
{
    constexpr Size kWindowSize{.width = 960, .height = 540};

    constexpr Size kCanvasSize{.width = 480, .height = 270};

    constexpr std::size_t kSomeIcons = 4;

    [[nodiscard]] Bitmap getBlankSheet(const std::size_t count)
    {
        return Bitmap{
            .size =
                {.width = static_cast<std::uint32_t>(
                     count * kIconCellSize.width),
                 .height = kIconCellSize.height},
            .pixels = std::vector<std::uint8_t>(
                count * kIconCellSize.width * kIconCellSize.height * 4, 0)};
    }

    void handsOutTextures(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createTexture(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const Bitmap &bitmap)
                {
                    return std::unique_ptr<ITexture>{
                        std::make_unique<NiceMock<MockTexture>>()};
                });
    }
}

TEST(IconsViewTest, Open_CarriesTheSheetAndItsBackingToPictures)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));

    EXPECT_NE(icons.getTexture(), nullptr);
    EXPECT_NE(icons.getChecker(), nullptr);
    EXPECT_EQ(icons.getCount(), kSomeIcons);
    EXPECT_FALSE(icons.isUnsaved());
}

TEST(IconsViewTest, Pick_TakesUpAnIconAndPutsItDown)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(2);

    EXPECT_EQ(icons.getPicked(), std::optional<std::size_t>{2});

    icons.pick(std::nullopt);

    EXPECT_FALSE(icons.getPicked().has_value());
}

TEST(IconsViewTest, Paint_ColorsThePixelAndLeavesTheSheetUnsaved)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(1);
    icons.paint(viewportRenderer, {.column = 3, .row = 4}, false);

    EXPECT_EQ(
        getIconPixelColor(icons.getSheet(), 1, {.column = 3, .row = 4})
            .alpha,
        255);
    EXPECT_TRUE(icons.isUnsaved());
}

TEST(IconsViewTest, Paint_ErasingClearsThePixelAgain)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(0);
    icons.paint(viewportRenderer, {.column = 1, .row = 1}, false);
    icons.paint(viewportRenderer, {.column = 1, .row = 1}, true);

    EXPECT_EQ(
        getIconPixelColor(icons.getSheet(), 0, {.column = 1, .row = 1})
            .alpha,
        0);
}

TEST(IconsViewTest, Keep_MarksWhatWasDrawnAsWritten)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(0);
    icons.paint(viewportRenderer, {.column = 0, .row = 0}, false);
    icons.keep();

    EXPECT_FALSE(icons.isUnsaved());
}

TEST(IconsViewTest, Draw_DrawsEveryIconAndTheOneTakenUp)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(1);

    EXPECT_CALL(innerRenderer, drawTexture)
        .Times(::testing::AtLeast(
            static_cast<int>(kSomeIcons) * 2));
    EXPECT_CALL(innerRenderer, drawRect)
        .Times(::testing::AtLeast(
            static_cast<int>(kIconCellSize.width * kIconCellSize.height)));

    icons.draw(viewportRenderer);
}

TEST(IconsViewTest, Draw_LeavesTheBlownUpIconOutWhereNoneIsTakenUp)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    IconsView icons;

    icons.open(viewportRenderer, getBlankSheet(kSomeIcons));
    icons.pick(std::nullopt);

    EXPECT_CALL(innerRenderer, drawRect).Times(0);

    icons.draw(viewportRenderer);
}
