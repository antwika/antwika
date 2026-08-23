#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <antwika/decor/TileAnimation.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/tilemap/Tilemap.hpp>

#include "antwika/render/AtlasSheets.hpp"

using antwika::render::AtlasSheets;
using antwika::gfx::Bitmap;
using antwika::gfx::ITexture;
using antwika::gfx::Size;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockTexture;
using antwika::tilemap::Atlas;
using ::testing::NiceMock;

namespace
{
    constexpr Size kWindowSize{.width = 960, .height = 540};

    constexpr Size kCanvasSize{.width = 480, .height = 270};

    [[nodiscard]] Bitmap sheetOf(
        const Size tileSize, const std::uint8_t shade)
    {
        const auto size = antwika::tilemap::getAtlasSize(tileSize);

        return Bitmap{
            .size = size,
            .pixels = std::vector<std::uint8_t>(
                static_cast<std::size_t>(size.width) * size.height
                    * antwika::gfx::kBytesPerPixel,
                shade)};
    }

    [[nodiscard]] std::array<Bitmap, 2> getBothSheets(
        const std::uint8_t shade)
    {
        return {
            sheetOf(antwika::tilemap::kWallTileSize, shade),
            sheetOf(antwika::tilemap::kFloorTileSize, shade)};
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

    [[nodiscard]] antwika::map::Map getMapWithFlippingWalls()
    {
        antwika::map::Map drawnMap;

        drawnMap.flipAnimations.push_back(
            antwika::decor::TileAnimation{
                .tile = {.atlas = Atlas::Wall, .index = 0},
                .frameTiles = {
                    {.atlas = Atlas::Wall, .index = 0},
                    {.atlas = Atlas::Wall, .index = 1}}});

        return drawnMap;
    }
}

TEST(AtlasSheetsTest, Open_CarriesBothSheetsAndTheirBackingsToPictures)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);

    EXPECT_NE(sheets.getTexture(Atlas::Wall), nullptr);
    EXPECT_NE(sheets.getTexture(Atlas::Floor), nullptr);
    EXPECT_NE(sheets.getKeyed(Atlas::Wall), nullptr);
    EXPECT_NE(sheets.getKeyed(Atlas::Floor), nullptr);
    EXPECT_NE(sheets.getChecker(Atlas::Wall), nullptr);
    EXPECT_NE(sheets.getChecker(Atlas::Floor), nullptr);
    EXPECT_FALSE(sheets.isTouched());
}

TEST(AtlasSheetsTest, Sheet_IsTheOneTheAtlasIsPaintedFrom)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);
    sheets.sheet(Atlas::Wall).pixels.front() = 11;
    sheets.sheet(Atlas::Floor).pixels.front() = 22;

    EXPECT_EQ(sheets.sheet(0U).pixels.front(), 11);
    EXPECT_EQ(sheets.sheet(1U).pixels.front(), 22);
    EXPECT_EQ(sheets.getSheets().size(), 2U);
}

TEST(AtlasSheetsTest, Take_LaysTheSheetsDownAndLeavesThemToTheRefresh)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);
    sheets.take(getBothSheets(9));

    EXPECT_TRUE(sheets.isTouched());
    EXPECT_EQ(sheets.sheet(Atlas::Wall).pixels.front(), 9);
}

TEST(AtlasSheetsTest, Refresh_MakesNoPictureWhereNothingWasPaintedOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);

    EXPECT_CALL(innerRenderer, createTexture).Times(0);

    sheets.refresh(viewportRenderer, drawnMap, 0, false);
}

TEST(AtlasSheetsTest, Refresh_MakesBothSheetsAfreshOncePaintedOn)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);
    sheets.touch();

    EXPECT_CALL(innerRenderer, createTexture).Times(4);

    sheets.refresh(viewportRenderer, drawnMap, 0, false);

    EXPECT_FALSE(sheets.isTouched());
}

TEST(AtlasSheetsTest, Refresh_LeavesASheetNoFlipWalksThroughAlone)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    ViewportRenderer viewportRenderer(innerRenderer, kWindowSize, kCanvasSize);
    AtlasSheets sheets;
    const auto drawnMap = getMapWithFlippingWalls();

    sheets.open(viewportRenderer, getBothSheets(3), drawnMap, 0);

    EXPECT_CALL(innerRenderer, createTexture).Times(2);

    sheets.refresh(viewportRenderer, drawnMap, 1, true);
}

TEST(AtlasSheetsTest, Open_TakesAnyRendererNotJustAViewportOne)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutTextures(innerRenderer);
    AtlasSheets sheets;
    const antwika::map::Map drawnMap;

    sheets.open(innerRenderer, getBothSheets(3), drawnMap, 0);

    EXPECT_NE(sheets.getTexture(Atlas::Wall), nullptr);
    EXPECT_NE(sheets.getTexture(Atlas::Floor), nullptr);
}
