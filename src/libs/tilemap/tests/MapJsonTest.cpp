#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/MapJson.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/TileMapError.hpp>

using antwika::geometry::GridCell;
using antwika::tilemap::MapDocument;
using antwika::tilemap::mapDocumentFromJson;
using antwika::tilemap::MapHeader;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::tileMapFromJson;
using antwika::tilemap::TileMapError;
using antwika::tilemap::toJson;

namespace
{
    [[nodiscard]] MapDocument documentOf()
    {
        MapDocument document{
            .map = TileMap(MapHeader{.id = "mask"}, 3, 2)};

        document.map.at(GridCell{.column = 1, .row = 0}).terrain =
            TerrainClass::Water;
        document.free = {false, true, false, true, false, true};

        return document;
    }
}

TEST(MapJsonTest, ToJson_AlwaysWritesSchemaTwoWithAFreeSection)
{
    TileMap map(
        MapHeader{.id = "plain", .schemaVersion = 1}, 2, 2);

    const auto out = toJson(map);

    EXPECT_EQ(out.at("schema").get<std::uint32_t>(), 2U);
    ASSERT_EQ(out.at("free").size(), 2U);
    EXPECT_EQ(out.at("free").at(0).get<std::string>(), "..");
    EXPECT_EQ(out.at("free").at(1).get<std::string>(), "..");
}

TEST(MapJsonTest, MapDocumentFromJson_RoundTripsTheFreeMask)
{
    const auto document = documentOf();
    const auto out = toJson(document);

    EXPECT_EQ(out.at("free").at(0).get<std::string>(), ".o.");
    EXPECT_EQ(out.at("free").at(1).get<std::string>(), "o.o");

    const auto loaded = mapDocumentFromJson(out);

    EXPECT_EQ(loaded.free, document.free);
    EXPECT_EQ(
        loaded.map.at(GridCell{.column = 1, .row = 0}).terrain,
        TerrainClass::Water);
}

TEST(MapJsonTest, MapDocumentFromJson_LoadsAVersionOneMapAllPinned)
{
    auto out = toJson(documentOf());

    out["schema"] = 1;
    out.erase("free");

    const auto loaded = mapDocumentFromJson(out);

    EXPECT_EQ(
        loaded.free,
        (std::vector<bool>{
            false, false, false, false, false, false}));
    EXPECT_EQ(loaded.map.header().schemaVersion, 1U);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsAMissingFreeSection)
{
    auto out = toJson(documentOf());

    out.erase("free");

    EXPECT_THROW((void)mapDocumentFromJson(out), TileMapError);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsWrongFreeDimensions)
{
    auto out = toJson(documentOf());

    out["free"] = nlohmann::json::array({".o.", "o."});

    EXPECT_THROW((void)mapDocumentFromJson(out), TileMapError);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsAnUnknownFreeMark)
{
    auto out = toJson(documentOf());

    out["free"] = nlohmann::json::array({".x.", "o.o"});

    EXPECT_THROW((void)mapDocumentFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAnUnknownSchemaVersion)
{
    auto out = toJson(documentOf());

    out["schema"] = 3;

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_LoadsAVersionTwoDocumentUntouched)
{
    const auto out = toJson(documentOf());
    const auto map = tileMapFromJson(out);

    EXPECT_EQ(map.columns(), 3U);
    EXPECT_EQ(map.rows(), 2U);
    EXPECT_EQ(
        map.at(GridCell{.column = 1, .row = 0}).terrain,
        TerrainClass::Water);
}
