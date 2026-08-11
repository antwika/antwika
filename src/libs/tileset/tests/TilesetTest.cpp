#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetError.hpp>

using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::internSocket;
using antwika::tileset::kDefaultDensity;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::kOpenSocket;
using antwika::tileset::removeLayer;
using antwika::tileset::removeSprite;
using antwika::tileset::SpriteId;
using antwika::tileset::Tileset;
using antwika::tileset::TilesetError;

namespace
{
    [[nodiscard]] Tileset mossyTileset()
    {
        Tileset set;

        static_cast<void>(addSprite(set, 0));
        static_cast<void>(addSprite(set, 0));
        static_cast<void>(addLayer(set, "moss"));
        static_cast<void>(addLayer(set, "vine"));

        addSprite(set, 1).on = {0, 1};
        addSprite(set, 2).on = {0};

        return set;
    }
}

TEST(TilesetTest, Tileset_DefaultsToTheReservedSocketsAndABaseLayer)
{
    const Tileset set;

    EXPECT_EQ(
        set.socketNames,
        (std::vector<std::string>{"edge", "open"}));
    EXPECT_EQ(set.socketNames[kEdgeSocket], "edge");
    EXPECT_EQ(set.socketNames[kOpenSocket], "open");
    ASSERT_EQ(set.layers.size(), 1U);
    EXPECT_EQ(set.layers[0].name, "base");
    EXPECT_EQ(set.layers[0].density, kDefaultDensity);
    EXPECT_EQ(set.nextSpriteId, 0U);
}

TEST(TilesetTest, InternSocket_AppendsANewNameAfterTheReservedPair)
{
    Tileset set;

    const auto flat = internSocket(set, "flat");

    EXPECT_EQ(flat, 2);
    EXPECT_EQ(
        set.socketNames,
        (std::vector<std::string>{"edge", "open", "flat"}));
}

TEST(TilesetTest, InternSocket_KeepsTheIndexOfAKnownName)
{
    Tileset set;

    const auto first = internSocket(set, "flat");
    const auto again = internSocket(set, "flat");

    EXPECT_EQ(first, 2);
    EXPECT_EQ(again, 2);
    EXPECT_EQ(internSocket(set, "edge"), kEdgeSocket);
    EXPECT_EQ(internSocket(set, "open"), kOpenSocket);
    EXPECT_EQ(set.socketNames.size(), 3U);
}

TEST(TilesetTest, AddSprite_AllocatesMonotonicIdsAcrossLayers)
{
    Tileset set;
    static_cast<void>(addLayer(set, "moss"));

    const auto first = addSprite(set, 0).id;
    const auto second = addSprite(set, 1).id;
    const auto third = addSprite(set, 0).id;

    EXPECT_EQ(first, 0U);
    EXPECT_EQ(second, 1U);
    EXPECT_EQ(third, 2U);
    EXPECT_EQ(set.nextSpriteId, 3U);
    EXPECT_EQ(set.layers[0].sprites.size(), 2U);
    EXPECT_EQ(set.layers[1].sprites.size(), 1U);
}

TEST(TilesetTest, AddSprite_NeverReusesARemovedId)
{
    Tileset set;
    static_cast<void>(addSprite(set, 0));

    removeSprite(set, 0, 0);

    EXPECT_EQ(addSprite(set, 0).id, 1U);
}

TEST(TilesetTest, AddSprite_RejectsALayerIndexPastTheLayers)
{
    Tileset set;

    EXPECT_THROW(
        static_cast<void>(addSprite(set, 1)), TilesetError);
}

TEST(TilesetTest, RemoveSprite_PrunesTheIdFromEveryDecorOnList)
{
    auto set = mossyTileset();

    removeSprite(set, 0, 0);

    ASSERT_EQ(set.layers[0].sprites.size(), 1U);
    EXPECT_EQ(set.layers[0].sprites[0].id, 1U);
    EXPECT_EQ(
        set.layers[1].sprites[0].on, (std::vector<SpriteId>{1}));
    EXPECT_TRUE(set.layers[2].sprites[0].on.empty());
}

TEST(TilesetTest, RemoveSprite_ErasesADecorSpriteWithoutPruning)
{
    auto set = mossyTileset();

    removeSprite(set, 1, 0);

    EXPECT_TRUE(set.layers[1].sprites.empty());
    EXPECT_EQ(
        set.layers[2].sprites[0].on, (std::vector<SpriteId>{0}));
}

TEST(TilesetTest, RemoveSprite_RejectsAnIndexPastTheSprites)
{
    auto set = mossyTileset();

    EXPECT_THROW(removeSprite(set, 0, 2), TilesetError);
}

TEST(TilesetTest, RemoveSprite_RejectsALayerIndexPastTheLayers)
{
    auto set = mossyTileset();

    EXPECT_THROW(removeSprite(set, 3, 0), TilesetError);
}

TEST(TilesetTest, AddLayer_AppendsANamedLayerWithTheDefaultDensity)
{
    Tileset set;

    static_cast<void>(addLayer(set, "moss"));

    ASSERT_EQ(set.layers.size(), 2U);
    const auto &moss = set.layers[1];
    EXPECT_EQ(moss.name, "moss");
    EXPECT_EQ(moss.density, kDefaultDensity);
    EXPECT_TRUE(moss.sprites.empty());
}

TEST(TilesetTest, RemoveLayer_ErasesTheLayerItNames)
{
    auto set = mossyTileset();

    removeLayer(set, 1);

    ASSERT_EQ(set.layers.size(), 2U);
    EXPECT_EQ(set.layers[0].name, "base");
    EXPECT_EQ(set.layers[1].name, "vine");
}

TEST(TilesetTest, RemoveLayer_RefusesTheBaseLayer)
{
    auto set = mossyTileset();

    EXPECT_THROW(removeLayer(set, 0), TilesetError);
}

TEST(TilesetTest, RemoveLayer_RejectsAnIndexPastTheLayers)
{
    auto set = mossyTileset();

    EXPECT_THROW(removeLayer(set, 3), TilesetError);
}

TEST(TilesetTest, Layer_OperatorEquals_ComparesEveryField)
{
    const antwika::tileset::Layer base{
        .name = "moss",
        .density = 32,
        .sprites = std::vector<antwika::tileset::Sprite>{{}}};

    EXPECT_EQ(base, base);

    auto other = base;
    other.name = "vine";
    EXPECT_NE(base, other);

    other = base;
    other.density = 33;
    EXPECT_NE(base, other);

    other = base;
    other.sprites.clear();
    EXPECT_NE(base, other);
}

TEST(TilesetTest, Tileset_OperatorEquals_ComparesEveryField)
{
    Tileset base;
    base.name = "rustwall";
    base.terrain = antwika::tilemap::TerrainClass::Wall;
    static_cast<void>(addSprite(base, 0));

    EXPECT_EQ(base, base);

    auto other = base;
    other.name = "mosswall";
    EXPECT_NE(base, other);

    other = base;
    other.terrain = antwika::tilemap::TerrainClass::Cliff;
    EXPECT_NE(base, other);

    other = base;
    other.nextSpriteId = 99;
    EXPECT_NE(base, other);

    other = base;
    static_cast<void>(internSocket(other, "rim"));
    EXPECT_NE(base, other);

    other = base;
    static_cast<void>(addLayer(other, "moss"));
    EXPECT_NE(base, other);
}
