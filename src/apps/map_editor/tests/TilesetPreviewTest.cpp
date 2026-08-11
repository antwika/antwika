#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>

#include "antwika/map_editor/TilesetPreview.hpp"

using antwika::map_editor::buildTilesetPreview;
using antwika::map_editor::kPreviewCenterColumn;
using antwika::map_editor::kPreviewCenterRow;
using antwika::map_editor::kPreviewColumns;
using antwika::map_editor::kPreviewRows;
using antwika::map_editor::TilesetPreview;
using antwika::tileset::kDefaultWeight;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::kOpenSocket;
using antwika::tileset::Layer;
using antwika::tileset::SocketId;
using antwika::tileset::Sprite;
using antwika::tileset::SpriteId;
using antwika::tileset::Tileset;

namespace
{
    constexpr SocketId kGrass = 2;
    constexpr SocketId kStone = 3;
    constexpr SocketId kLink = 4;

    constexpr std::uint32_t kSeed = 7;

    constexpr std::uint32_t kZeroingSeed = 0x9E3779B9U;

    [[nodiscard]] std::size_t cellAt(
        const std::int32_t column, const std::int32_t row)
    {
        return static_cast<std::size_t>(
            row * kPreviewColumns + column);
    }

    [[nodiscard]] std::size_t centerCell()
    {
        return cellAt(kPreviewCenterColumn, kPreviewCenterRow);
    }

    [[nodiscard]] Sprite spriteOf(
        const SpriteId id,
        const std::array<SocketId, 4> &sockets,
        const std::vector<SpriteId> &on = {})
    {
        Sprite sprite{};
        sprite.id = id;
        sprite.sockets = sockets;
        sprite.on = on;

        return sprite;
    }

    [[nodiscard]] Sprite evenSprite(
        const SpriteId id,
        const SocketId socket,
        const std::uint8_t weight = kDefaultWeight)
    {
        auto sprite =
            spriteOf(id, {socket, socket, socket, socket});
        sprite.weight = weight;

        return sprite;
    }

    [[nodiscard]] Tileset setOf(const std::vector<Sprite> &base)
    {
        Tileset data{};
        data.layers[0].sprites = base;

        return data;
    }

    void addDecor(
        Tileset &data,
        const std::uint8_t density,
        const std::vector<Sprite> &sprites)
    {
        Layer layer{};
        layer.name = "decor";
        layer.density = density;
        layer.sprites = sprites;
        data.layers.push_back(layer);
    }

    [[nodiscard]] std::int32_t decorCount(
        const TilesetPreview &preview, const std::size_t layer)
    {
        std::int32_t held = 0;

        for (const auto sprite : preview.decor[layer])
        {
            held += sprite >= 0 ? 1 : 0;
        }

        return held;
    }

    [[nodiscard]] bool fillsOnlyInside(const TilesetPreview &preview)
    {
        for (std::size_t at = 0; at < preview.base.size(); ++at)
        {
            if (preview.outside[at] == (preview.base[at] >= 0))
            {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool decorHolds(
        const TilesetPreview &preview,
        const std::size_t layer,
        const std::int32_t sprite)
    {
        for (const auto held : preview.decor[layer])
        {
            if (held == sprite)
            {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool holdsSprite(
        const TilesetPreview &preview, const std::int32_t sprite)
    {
        for (const auto held : preview.base)
        {
            if (held == sprite)
            {
                return true;
            }
        }

        return false;
    }
}

TEST(TilesetPreviewTest, BuildTilesetPreview_PinsTheSelectedBase)
{
    const auto data =
        setOf({evenSprite(1, kGrass), evenSprite(2, kGrass)});

    const auto preview = buildTilesetPreview(data, 0, 1, kSeed);

    EXPECT_EQ(preview.base[centerCell()], 1);
    EXPECT_FALSE(preview.centerBaseMissing);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_FillsEveryCellWithoutAnEdge)
{
    const auto data = setOf({evenSprite(1, kGrass)});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    for (std::size_t at = 0; at < preview.base.size(); ++at)
    {
        EXPECT_FALSE(preview.outside[at]);
        EXPECT_EQ(preview.base[at], 0);
    }
}

TEST(TilesetPreviewTest, BuildTilesetPreview_KeepsOneCellInsideFourEdges)
{
    const auto data = setOf({evenSprite(1, kEdgeSocket)});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    EXPECT_FALSE(preview.outside[centerCell()]);
    EXPECT_EQ(preview.base[centerCell()], 0);

    for (std::size_t at = 0; at < preview.base.size(); ++at)
    {
        if (at != centerCell())
        {
            EXPECT_TRUE(preview.outside[at]);
            EXPECT_EQ(preview.base[at], -1);
        }
    }
}

TEST(TilesetPreviewTest, BuildTilesetPreview_MarksCellsBeyondTheEdges)
{
    const auto data = setOf({spriteOf(
        1, {kEdgeSocket, kGrass, kGrass, kEdgeSocket})});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    EXPECT_TRUE(preview.outside[cellAt(5, 1)]);
    EXPECT_TRUE(preview.outside[cellAt(4, 2)]);
    EXPECT_TRUE(preview.outside[cellAt(0, 0)]);
    EXPECT_FALSE(preview.outside[cellAt(5, 2)]);
    EXPECT_FALSE(preview.outside[cellAt(10, 2)]);
    EXPECT_FALSE(preview.outside[cellAt(5, 4)]);
    EXPECT_TRUE(fillsOnlyInside(preview));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_SpreadsTheMatchingNeighbor)
{
    const auto data = setOf(
        {spriteOf(1, {kEdgeSocket, kGrass, kEdgeSocket, kGrass}),
         spriteOf(2, {kEdgeSocket, kStone, kEdgeSocket, kStone})});

    const auto preview = buildTilesetPreview(data, 0, 1, kSeed);

    for (std::int32_t column = 0; column < kPreviewColumns; ++column)
    {
        EXPECT_EQ(preview.base[cellAt(column, kPreviewCenterRow)], 1);
    }

    EXPECT_FALSE(holdsSprite(preview, 0));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_FillsCellsNoSpriteCanSatisfy)
{
    const auto data =
        setOf({evenSprite(1, kGrass), evenSprite(2, kStone)});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    EXPECT_TRUE(holdsSprite(preview, 0));
    EXPECT_TRUE(holdsSprite(preview, 1));
    EXPECT_TRUE(fillsOnlyInside(preview));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_FavorsTheHeavierSprite)
{
    const auto data = setOf(
        {evenSprite(1, kGrass, 1), evenSprite(2, kGrass, 15)});
    std::int32_t light = 0;
    std::int32_t heavy = 0;

    for (std::uint32_t seed = 0; seed < 50; ++seed)
    {
        const auto preview = buildTilesetPreview(data, 0, 0, seed);

        for (const auto held : preview.base)
        {
            light += held == 0 ? 1 : 0;
            heavy += held == 1 ? 1 : 0;
        }
    }

    EXPECT_GT(heavy, light * 3);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_VariesOnTheZeroingSeed)
{
    const auto data =
        setOf({evenSprite(1, kGrass), evenSprite(2, kGrass)});

    const auto preview =
        buildTilesetPreview(data, 0, 0, kZeroingSeed);

    EXPECT_TRUE(holdsSprite(preview, 0));
    EXPECT_TRUE(holdsSprite(preview, 1));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_EmptiesAMissingLayer)
{
    auto data = setOf({evenSprite(1, kGrass)});
    addDecor(data, 255, {spriteOf(2, {1, 1, 1, 1}, {1})});

    const auto preview = buildTilesetPreview(data, 3, 0, kSeed);

    EXPECT_EQ(preview.base[centerCell()], -1);
    EXPECT_EQ(decorCount(preview, 0), 0);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_EmptiesAMissingSprite)
{
    const auto data = setOf({evenSprite(1, kGrass)});

    const auto preview = buildTilesetPreview(data, 0, 4, kSeed);

    EXPECT_EQ(preview.base[centerCell()], -1);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_EmptiesAnEmptyBaseLayer)
{
    Tileset data{};
    addDecor(data, 255, {spriteOf(1, {1, 1, 1, 1}, {7})});

    const auto preview = buildTilesetPreview(data, 1, 0, kSeed);

    EXPECT_EQ(preview.base[centerCell()], -1);
    EXPECT_EQ(decorCount(preview, 0), 0);
    EXPECT_FALSE(preview.centerBaseMissing);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_PinsDecorOnItsOwnLayer)
{
    auto data = setOf({evenSprite(1, kGrass)});
    addDecor(
        data,
        0,
        {spriteOf(
            2,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {1})});
    addDecor(
        data,
        0,
        {spriteOf(
            3,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {1})});

    const auto preview = buildTilesetPreview(data, 1, 0, kSeed);

    EXPECT_EQ(preview.decor[0][centerCell()], 0);
    EXPECT_EQ(decorCount(preview, 0), 1);
    EXPECT_EQ(decorCount(preview, 1), 0);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_ReportsDecorOnNoBase)
{
    auto data = setOf({evenSprite(1, kGrass)});
    addDecor(
        data,
        255,
        {spriteOf(
            2,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {9})});

    const auto preview = buildTilesetPreview(data, 1, 0, kSeed);

    EXPECT_TRUE(preview.centerBaseMissing);
    EXPECT_EQ(preview.decor[0][centerCell()], -1);
    EXPECT_EQ(decorCount(preview, 0), 0);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_FillsAShapeNoBaseHas)
{
    auto data =
        setOf({evenSprite(1, kGrass), evenSprite(2, kStone)});
    addDecor(
        data,
        0,
        {spriteOf(
            3,
            {kEdgeSocket, kOpenSocket, kEdgeSocket, kOpenSocket},
            {1})});

    const auto preview = buildTilesetPreview(data, 1, 0, kSeed);

    for (std::int32_t column = 0; column < kPreviewColumns; ++column)
    {
        EXPECT_EQ(preview.base[cellAt(column, kPreviewCenterRow)], 0);
    }

    EXPECT_TRUE(preview.outside[cellAt(5, 1)]);
    EXPECT_FALSE(preview.centerBaseMissing);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_ScattersMoreAtAHighDensity)
{
    auto sparse = setOf({evenSprite(1, kGrass)});
    addDecor(
        sparse,
        32,
        {spriteOf(
            2,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {1})});
    auto dense = sparse;
    dense.layers[1].density = 224;

    const auto few = buildTilesetPreview(sparse, 0, 0, kSeed);
    const auto many = buildTilesetPreview(dense, 0, 0, kSeed);

    EXPECT_LT(decorCount(few, 0), decorCount(many, 0));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_PlacesDecorOnItsOwnBase)
{
    auto data =
        setOf({evenSprite(1, kGrass), evenSprite(2, kGrass)});
    addDecor(
        data,
        255,
        {spriteOf(
            3,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {1})});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    ASSERT_TRUE(holdsSprite(preview, 1));

    for (std::size_t at = 0; at < preview.base.size(); ++at)
    {
        if (preview.base[at] == 1)
        {
            EXPECT_EQ(preview.decor[0][at], -1);
        }
        else
        {
            EXPECT_EQ(preview.decor[0][at], 0);
        }
    }
}

TEST(TilesetPreviewTest, BuildTilesetPreview_ContinuesADecorChain)
{
    auto data = setOf({evenSprite(1, kGrass)});
    addDecor(
        data,
        0,
        {spriteOf(2, {kOpenSocket, kLink, kLink, kOpenSocket}, {1}),
         spriteOf(3, {kOpenSocket, kOpenSocket, kOpenSocket, kLink},
                  {1}),
         spriteOf(4, {kLink, kOpenSocket, kOpenSocket, kOpenSocket},
                  {1})});

    const auto preview = buildTilesetPreview(data, 1, 0, kSeed);

    EXPECT_EQ(preview.decor[0][centerCell()], 0);
    EXPECT_EQ(preview.decor[0][cellAt(6, 2)], 1);
    EXPECT_EQ(preview.decor[0][cellAt(5, 3)], 2);
    EXPECT_EQ(preview.decor[0][cellAt(7, 2)], -1);
    EXPECT_EQ(preview.decor[0][cellAt(6, 3)], -1);
}

TEST(TilesetPreviewTest, BuildTilesetPreview_DropsDecorPastTheEdge)
{
    auto data = setOf(
        {spriteOf(1, {kGrass, kEdgeSocket, kEdgeSocket, kGrass})});
    addDecor(
        data,
        255,
        {spriteOf(2, {kOpenSocket, kLink, kLink, kOpenSocket}, {1}),
         spriteOf(
             3,
             {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
             {1})});

    constexpr std::int32_t kSpur = 0;
    constexpr std::int32_t kDot = 1;

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    for (std::int32_t column = 0; column <= 5; ++column)
    {
        EXPECT_NE(preview.decor[0][cellAt(column, 2)], kSpur);
    }

    for (std::int32_t row = 0; row <= 2; ++row)
    {
        EXPECT_NE(preview.decor[0][cellAt(5, row)], kSpur);
    }

    EXPECT_EQ(preview.decor[0][cellAt(5, 2)], kDot);
    EXPECT_TRUE(decorHolds(preview, 0, kSpur));
}

TEST(TilesetPreviewTest, BuildTilesetPreview_PlacesNothingForNoSprites)
{
    auto data = setOf({evenSprite(1, kGrass)});
    addDecor(data, 255, {});
    addDecor(
        data,
        255,
        {spriteOf(
            2,
            {kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket},
            {1})});

    const auto preview = buildTilesetPreview(data, 0, 0, kSeed);

    EXPECT_EQ(decorCount(preview, 0), 0);
    EXPECT_GT(decorCount(preview, 1), 0);
}
