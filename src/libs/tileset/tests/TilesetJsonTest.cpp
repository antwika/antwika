#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Sprite.hpp>
#include <antwika/tileset/Tileset.hpp>
#include <antwika/tileset/TilesetError.hpp>
#include <antwika/tileset/TilesetJson.hpp>

using antwika::tilemap::TerrainClass;
using antwika::tileset::addLayer;
using antwika::tileset::addSprite;
using antwika::tileset::internSocket;
using antwika::tileset::kDefaultDensity;
using antwika::tileset::kEdgeSocket;
using antwika::tileset::SocketId;
using antwika::tileset::SpriteId;
using antwika::tileset::Tileset;
using antwika::tileset::TilesetError;
using antwika::tileset::tilesetFromJson;
using antwika::tileset::toJson;

namespace
{
    [[nodiscard]] Tileset rustwall()
    {
        Tileset set;
        set.name = "rustwall";
        set.terrain = TerrainClass::Wall;

        const auto flat = internSocket(set, "flat");

        addSprite(set, 0).sockets = {
            kEdgeSocket, flat, flat, kEdgeSocket};
        static_cast<void>(addSprite(set, 0));

        addLayer(set, "moss").density = 48;

        auto &tuft = addSprite(set, 1);
        tuft.frameCount = 2;
        tuft.on = {1};

        return set;
    }

    constexpr std::string_view kRustwallBytes = R"({
  "schema": 1,
  "name": "rustwall",
  "terrain": "wall",
  "nextSpriteId": 3,
  "layers": [
    {
      "name": "base",
      "sprites": [
        {
          "id": 0,
          "sockets": {
            "n": "edge",
            "e": "flat",
            "s": "flat",
            "w": "edge"
          },
          "on": []
        },
        {
          "id": 1,
          "sockets": {
            "n": "open",
            "e": "open",
            "s": "open",
            "w": "open"
          },
          "on": []
        }
      ]
    },
    {
      "name": "moss",
      "density": 48,
      "sprites": [
        {
          "id": 2,
          "frames": 2,
          "sockets": {
            "n": "open",
            "e": "open",
            "s": "open",
            "w": "open"
          },
          "on": [
            1
          ]
        }
      ]
    }
  ]
})";

    [[nodiscard]] nlohmann::ordered_json rustwallDocument()
    {
        return nlohmann::ordered_json::parse(toJson(rustwall()));
    }
}

TEST(TilesetJsonTest, ToJson_WritesTheCanonicalTilesetByteForByte)
{
    EXPECT_EQ(toJson(rustwall()), kRustwallBytes);
}

TEST(TilesetJsonTest, ToJson_OmitsDensityOnTheBaseLayer)
{
    auto set = rustwall();
    set.layers[0].density = 90;

    const auto document =
        nlohmann::ordered_json::parse(toJson(set));

    EXPECT_FALSE(document["layers"][0].contains("density"));
}

TEST(TilesetJsonTest, ToJson_OmitsADecorDensityAtTheDefault)
{
    auto set = rustwall();
    set.layers[1].density = kDefaultDensity;

    const auto document =
        nlohmann::ordered_json::parse(toJson(set));

    EXPECT_FALSE(document["layers"][1].contains("density"));
}

TEST(TilesetJsonTest, ToJson_RejectsASocketOutsideTheInternTable)
{
    auto set = rustwall();
    set.layers[0].sprites[0].sockets[0] = SocketId{99};

    EXPECT_THROW((void)toJson(set), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RoundTripsTheCanonicalTileset)
{
    const auto set = rustwall();

    const auto loaded = tilesetFromJson(toJson(set));

    ASSERT_EQ(loaded.layers.size(), 2U);
    EXPECT_EQ(loaded, set);
    EXPECT_EQ(toJson(loaded), kRustwallBytes);
}

TEST(TilesetJsonTest, TilesetFromJson_RoundTripsEveryTerrainName)
{
    for (const auto terrain :
         antwika::enums::kAll<TerrainClass>)
    {
        Tileset set;
        set.terrain = terrain;

        EXPECT_EQ(
            tilesetFromJson(toJson(set)).terrain, terrain)
            << toString(terrain);
    }
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsTextThatIsNotJson)
{
    EXPECT_THROW(
        (void)tilesetFromJson("not a tileset"), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonObjectDocument)
{
    EXPECT_THROW((void)tilesetFromJson("[]"), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAnUnknownSchemaVersion)
{
    auto document = rustwallDocument();
    document["schema"] = 2;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonIntegerSchema)
{
    auto document = rustwallDocument();
    document["schema"] = "one";

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAMissingName)
{
    auto document = rustwallDocument();
    document.erase("name");

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonStringName)
{
    auto document = rustwallDocument();
    document["name"] = 7;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAnUnknownTerrain)
{
    auto document = rustwallDocument();
    document["terrain"] = "lava";

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANextSpriteIdPastRange)
{
    auto document = rustwallDocument();
    document["nextSpriteId"] = 4294967296;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAnUnsignedOverflowingId)
{
    auto document = rustwallDocument();
    document["nextSpriteId"] = 18446744073709551615ULL;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsATilesetWithoutLayers)
{
    auto document = rustwallDocument();
    document["layers"] = nlohmann::ordered_json::array();

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsLayersThatAreNoArray)
{
    auto document = rustwallDocument();
    document["layers"] = 5;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsSpritesThatAreNoArray)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"] = 5;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsZeroFrames)
{
    auto document = rustwallDocument();
    document["layers"][1]["sprites"][0]["frames"] = 0;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsFiveFrames)
{
    auto document = rustwallDocument();
    document["layers"][1]["sprites"][0]["frames"] = 5;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_DefaultsFramesToOne)
{
    const auto loaded = tilesetFromJson(toJson(rustwall()));

    EXPECT_EQ(loaded.layers[0].sprites[0].frameCount, 1);
    EXPECT_EQ(loaded.layers[1].sprites[0].frameCount, 2);
}

TEST(TilesetJsonTest, ToJson_RoundTripsANonDefaultWeight)
{
    auto set = rustwall();
    set.layers[0].sprites[1].weight = 16;

    const auto loaded = tilesetFromJson(toJson(set));

    EXPECT_EQ(loaded, set);
    EXPECT_EQ(loaded.layers[0].sprites[1].weight, 16);
}

TEST(TilesetJsonTest, ToJson_OmitsTheDefaultWeight)
{
    const auto document = rustwallDocument();

    for (const auto &layer : document["layers"])
    {
        for (const auto &sprite : layer["sprites"])
        {
            EXPECT_FALSE(sprite.contains("weight"));
        }
    }
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAZeroWeight)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["weight"] = 0;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsASeventeenWeight)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["weight"] = 17;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsADensityPastTheByte)
{
    auto document = rustwallDocument();
    document["layers"][1]["density"] = 256;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_IgnoresADensityOnTheBaseLayer)
{
    auto document = rustwallDocument();
    document["layers"][0]["density"] = 48;

    const auto loaded = tilesetFromJson(document.dump());

    EXPECT_EQ(loaded.layers[0].density, kDefaultDensity);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAMissingSocketKey)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["sockets"].erase("w");

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAnEmptySocketName)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["sockets"]["n"] = "";

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonStringSocketName)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["sockets"]["n"] = 3;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsNonObjectSockets)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["sockets"] = 5;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_InternsSocketNamesFirstSeen)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][1]["sockets"]["n"] = "rim";

    const auto loaded = tilesetFromJson(document.dump());

    EXPECT_EQ(
        loaded.socketNames,
        (std::vector<std::string>{
            "edge", "open", "flat", "rim"}));
    EXPECT_EQ(loaded.layers[0].sprites[1].sockets[0], 3);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonEmptyBaseOnList)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["on"] = {1};

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonArrayBaseOnList)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0]["on"] = 3;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANonArrayDecorOnList)
{
    auto document = rustwallDocument();
    document["layers"][1]["sprites"][0]["on"] = 3;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_DropsAnOnIdNamingNoBaseSprite)
{
    auto document = rustwallDocument();
    document["layers"][1]["sprites"][0]["on"] = {1, 9};

    const auto loaded = tilesetFromJson(document.dump());

    EXPECT_EQ(
        loaded.layers[1].sprites[0].on,
        (std::vector<SpriteId>{1}));
}

TEST(TilesetJsonTest, TilesetFromJson_AcceptsAMissingOnList)
{
    auto document = rustwallDocument();
    document["layers"][0]["sprites"][0].erase("on");
    document["layers"][1]["sprites"][0].erase("on");

    const auto loaded = tilesetFromJson(document.dump());

    EXPECT_TRUE(loaded.layers[1].sprites[0].on.empty());
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsADuplicateSpriteId)
{
    auto document = rustwallDocument();
    document["layers"][1]["sprites"][0]["id"] = 0;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAStaleNextSpriteId)
{
    auto document = rustwallDocument();
    document["nextSpriteId"] = 2;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsAWholeBeyondTheSignedTop)
{
    auto document = rustwallDocument();
    document["schema"] = std::numeric_limits<std::uint64_t>::max();

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}

TEST(TilesetJsonTest, TilesetFromJson_RejectsANegativeWhole)
{
    auto document = rustwallDocument();
    document["nextSpriteId"] = -1;

    EXPECT_THROW(
        (void)tilesetFromJson(document.dump()), TilesetError);
}
