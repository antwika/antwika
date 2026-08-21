#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>

#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>

using antwika::tilemap::Atlas;
using antwika::tilemap::defaultTilemap;
using antwika::voxel::EdgeKind;
using antwika::tilemap::kEveryTileEdge;
using antwika::map::kMaxCellCoord;
using antwika::map::loadMap;
using antwika::map::Map;
using antwika::map::MapFileError;
using antwika::map::serializeMap;
using antwika::voxelmap::demoCells;
using antwika::map::readMap;
using antwika::map::saveMap;
using antwika::voxel::Side;
using antwika::tilemap::swapTiles;
using antwika::tilemap::Tile;
using antwika::tilemap::TileEdge;
using antwika::voxel::VoxelCell;

namespace
{
    [[nodiscard]] Map readText(const std::string &text)
    {
        std::istringstream inputStream(text);

        return readMap(inputStream);
    }

    void ageTo(
        nlohmann::json &document, const std::uint32_t version)
    {
        if (version < 42)
        {
            document.erase(std::string("food"));
            document.erase(std::string("water"));
        }

        if (version < 41)
        {
            document["figures"] = document["characters"];
            document.erase(std::string("characters"));

            for (auto &figure : document["figures"])
            {
                figure.erase(std::string("player"));
            }

            document["walker"] = nlohmann::json();
        }

        document["version"] = version;
    }

    [[nodiscard]] Map demoMap()
    {
        return Map{
            .voxels = demoCells(), .tilemap = defaultTilemap()};
    }

    [[nodiscard]] std::string aroundTiles(const std::string &tiles)
    {
        return R"({"magic": "antwika.map", "version": 1,
                   "voxels": [], "tilemap": )"
               + tiles + "}";
    }

    [[nodiscard]] bool holdsAFloat(const nlohmann::json &value)
    {
        if (value.is_number_float())
        {
            return true;
        }

        if (value.is_array() || value.is_object())
        {
            for (const auto &item : value)
            {
                if (holdsAFloat(item))
                {
                    return true;
                }
            }
        }

        return false;
    }

    [[nodiscard]] std::string somewhereToWrite(
        const std::string &name)
    {
        const auto path =
            std::filesystem::temp_directory_path() / name;

        return path.string();
    }
}

TEST(MapFileTest, Map_ReadsBackTheMapItWrote)
{
    const auto map = demoMap();

    EXPECT_EQ(readText(serializeMap(map)), map);
}

TEST(MapFileTest, Map_ReadsBackAMapHoldingNothingAtAll)
{
    const Map bareMap;

    EXPECT_EQ(readText(serializeMap(bareMap)), bareMap);
}

TEST(MapFileTest, Map_KeepsHowTheTilesHaveBeenArranged)
{
    auto map = demoMap();

    swapTiles(map.tilemap, {.column = 0, .row = 0},
              {.column = 31, .row = 15});
    swapTiles(map.tilemap, {.column = 4, .row = 2},
              {.column = 5, .row = 2});

    const auto reloadedMap = readText(serializeMap(map));

    EXPECT_EQ(reloadedMap, map);
    EXPECT_EQ(reloadedMap.tilemap.at(0, 0), *map.tilemap.at(0, 0));
    EXPECT_NE(reloadedMap.tilemap.at(0, 0), defaultTilemap().at(0, 0));
}

TEST(MapFileTest, Map_KeepsItsDecorAndItsSpawnCubeAndItsExitCube)
{
    Map map;
    antwika::decor::DecorTile decor{
        .tile = {.atlas = antwika::tilemap::Atlas::Floor, .index = 3},
        .frameTiles =
            {{.atlas = antwika::tilemap::Atlas::Floor, .index = 3},
             {.atlas = antwika::tilemap::Atlas::Floor, .index = 4}},
        .allowedBaseTiles =
            {{.atlas = antwika::tilemap::Atlas::Floor, .index = 7}},
        .frequency = 35,
        .spanTiles =
            {{.atlas = antwika::tilemap::Atlas::Floor, .index = 3}}};

    map.decor = {decor};
    map.decorRules.allow(
        decor.tile,
        antwika::tilemap::TileEdge{},
        {.atlas = antwika::tilemap::Atlas::Floor, .index = 9});
    map.spawnCubeCell = VoxelCell{.x = 2, .y = 0, .z = -2};
    map.exitCubeCell = VoxelCell{.x = -4, .y = 2, .z = 6};

    const auto reloadedMap = readText(serializeMap(map));

    EXPECT_EQ(reloadedMap, map);
    EXPECT_EQ(reloadedMap.decor, map.decor);
    EXPECT_EQ(reloadedMap.decorRules, map.decorRules);
    EXPECT_EQ(reloadedMap.spawnCubeCell, map.spawnCubeCell);
    EXPECT_EQ(reloadedMap.exitCubeCell, map.exitCubeCell);
}

TEST(MapFileTest, Map_KeepsThePlacesItWasGiven)
{
    Map map;

    map.voxels = {
        VoxelCell{.x = -kMaxCellCoord, .y = 0, .z = kMaxCellCoord},
        VoxelCell{.x = 7, .y = -3, .z = 11}};

    EXPECT_EQ(readText(serializeMap(map)).voxels, map.voxels);
}

TEST(MapFileTest, HoldsAFloat_TellsAFloatFromAWholeNumber)
{
    EXPECT_TRUE(holdsAFloat(nlohmann::json::parse(
        R"({"voxels": [[1, 2.5, 3]]})")));
    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(
        R"({"voxels": [[1, 2, 3]]})")));
}

TEST(MapFileTest, Map_WritesNoFloatAtAll)
{
    const auto documentJson = nlohmann::json::parse(serializeMap(demoMap()));

    EXPECT_FALSE(holdsAFloat(documentJson));
}

TEST(MapFileTest, Map_SaysWhatItIsAndWhichVersionItIs)
{
    const auto text = serializeMap(Map{});

    EXPECT_NE(text.find("antwika.map"), std::string::npos);
    EXPECT_NE(text.find("\"version\""), std::string::npos);
}

TEST(MapFileTest, ReadMap_TurnsAwayTextThatIsNotJson)
{
    EXPECT_THROW((void)readText("this is not json"), MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAFileOfAnotherKind)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.voxels", "version": 1,
                "voxels": [], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAVersionItDoesNotKnow)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 99,
                "voxels": [], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithNoTilemap)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": []})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithNoVoxels)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAPlaceOfTheWrongLength)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [[1, 2]], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAPlaceThatIsNotAWholeNumber)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [[1.5, 2, 3]], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAPlaceTooFarOut)
{
    const auto pastEndText = std::to_string(
        static_cast<std::int64_t>(kMaxCellCoord) + 1);

    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [[)"
            + pastEndText
            + R"(, 0, 0]], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAnAtlasThereIsNot)
{
    EXPECT_THROW(
        (void)readText(
            aroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["sideways", 0]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayATileOfATilesThereIsNot)
{
    EXPECT_THROW(
        (void)readText(
            aroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["flat", 256]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayATileOfTheWrongLength)
{
    EXPECT_THROW(
        (void)readText(
            aroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["flat"]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAGridItHoldsTooFewTilesFor)
{
    EXPECT_THROW(
        (void)readText(
            aroundTiles(
                R"({"columns": 8, "rows": 8,
                    "tiles": [["flat", 0]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TakesAGridItHoldsEveryTileFor)
{
    const auto map = readText(
        aroundTiles(
            R"({"columns": 2, "rows": 1,
                "tiles": [["flat", 3], ["upright", 4]]})"));

    ASSERT_TRUE(map.tilemap.isComplete());
    EXPECT_EQ(map.tilemap.at(0, 0)->index, 3U);
    EXPECT_EQ(map.tilemap.at(1, 0)->index, 4U);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMemberItDoesNotKnow)
{
    EXPECT_THROW(
        (void)readText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [], "surprise": 1, "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, SaveMap_WritesAMapLoadMapReadsBack)
{
    const auto path = somewhereToWrite("antwika-map.json");
    const auto map = demoMap();

    saveMap(path, map);

    EXPECT_EQ(loadMap(path), map);

    std::filesystem::remove(path);
}

TEST(MapFileTest, SaveMap_CarriesTheRulesThroughAFile)
{
    const auto path = somewhereToWrite("antwika-ruled-map.json");
    auto map = demoMap();

    map.rules.allow(
        *map.tilemap.at(2, 0),
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary},
        *map.tilemap.at(5, 3));
    map.rules.allow(
        *map.tilemap.at(2, 0),
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary},
        *map.tilemap.at(6, 3));

    saveMap(path, map);

    const auto loadedMap = loadMap(path);

    EXPECT_EQ(loadedMap, map);
    EXPECT_EQ(loadedMap.rules, map.rules);
    EXPECT_EQ(loadedMap.rules.size(), 2U);

    std::filesystem::remove(path);
}

TEST(MapFileTest, LoadMap_SaysSoWhenThereIsNoSuchFile)
{
    EXPECT_THROW(
        (void)loadMap(somewhereToWrite("antwika-no-such-map.json")),
        MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheRulesThroughAWrittenMap)
{
    Map map{.voxels = demoCells(), .tilemap = defaultTilemap()};
    constexpr Tile wallTile{.atlas = Atlas::Wall, .index = 7};
    constexpr TileEdge aboveEdge{
        .side = Side::Top, .edge = EdgeKind::Boundary};

    map.rules.allow(*map.tilemap.at(0, 0), aboveEdge, wallTile);
    map.rules.allow(*map.tilemap.at(0, 0), aboveEdge, *map.tilemap.at(1, 0));
    map.rules.allow(
        *map.tilemap.at(4, 2),
        TileEdge{.side = Side::Right, .edge = EdgeKind::Interior},
        wallTile);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(loadedMap.rules, map.rules);
    EXPECT_EQ(loadedMap.rules.size(), 3U);
    EXPECT_TRUE(
        loadedMap.rules.allows(*map.tilemap.at(0, 0), aboveEdge, wallTile));
}

TEST(MapFileTest, WriteMap_KeepsTheRulesOfEveryEdgeApart)
{
    Map map{.tilemap = defaultTilemap()};
    constexpr Tile oneTile{.atlas = Atlas::Wall, .index = 1};
    constexpr Tile otherTile{.atlas = Atlas::Floor, .index = 2};

    for (const auto edge : kEveryTileEdge)
    {
        map.rules.allow(oneTile, edge, otherTile);
    }

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(loadedMap.rules, map.rules);

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_TRUE(loadedMap.rules.allows(oneTile, edge, otherTile));
    }
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenOnceRulesAreKept)
{
    Map map{.voxels = demoCells(), .tilemap = defaultTilemap()};

    map.rules.allow(
        *map.tilemap.at(3, 1),
        TileEdge{.side = Side::Left, .edge = EdgeKind::Interior},
        *map.tilemap.at(9, 4));

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(serializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoRulesYet)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("rules"));
    ageTo(document, 1);

    const auto loadedMap = readText(document.dump());

    EXPECT_EQ(loadedMap.rules.size(), 0U);
    EXPECT_EQ(loadedMap.tilemap, defaultTilemap());
}

TEST(MapFileTest, ReadMap_RefusesARuleForAnEdgeThatIsNot)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["rules"].push_back(
        {{"tile", {"upright", 0}},
         {"side", "sideways"},
         {"edge", "outward"},
         {"may", {{"flat", 1}}}});

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesAnEdgeShutAgainstEverything)
{
    Map map{.tilemap = defaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};
    constexpr TileEdge shutEdge{
        .side = Side::Left, .edge = EdgeKind::Interior};
    constexpr TileEdge openEdge{
        .side = Side::Right, .edge = EdgeKind::Interior};

    map.rules.forbidAll(tile, shutEdge);
    map.rules.allow(tile, openEdge, tile);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_TRUE(loadedMap.rules.isForbidden(tile, shutEdge));
    EXPECT_FALSE(loadedMap.rules.hasNoRule(tile, shutEdge));
    EXPECT_FALSE(loadedMap.rules.isForbidden(tile, openEdge));
    EXPECT_TRUE(loadedMap.rules.allows(tile, openEdge, tile));
    EXPECT_EQ(loadedMap.rules, map.rules);
}

TEST(MapFileTest, ReadMap_TellsAShutEdgeFromOneNeverSpokenOf)
{
    Map map{.tilemap = defaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};
    constexpr TileEdge shutEdge{
        .side = Side::Left, .edge = EdgeKind::Interior};
    constexpr TileEdge neverEdge{
        .side = Side::Top, .edge = EdgeKind::Interior};

    map.rules.forbidAll(tile, shutEdge);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_TRUE(loadedMap.rules.isForbidden(tile, shutEdge));
    EXPECT_TRUE(loadedMap.rules.hasNoRule(tile, neverEdge));
    EXPECT_FALSE(loadedMap.rules.isForbidden(tile, neverEdge));
}

TEST(MapFileTest, ReadMap_RefusesTheSameNeighbourAllowedTwice)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["rules"].push_back(
        {{"tile", {"upright", 0}},
         {"side", "top"},
         {"edge", "outward"},
         {"may", {{"flat", 1}, {"flat", 1}}}});

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesOneEdgeOfOneTileSpokenOfTwice)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));
    const nlohmann::json rule = {
        {"tile", {"upright", 0}},
        {"side", "top"},
        {"edge", "outward"},
        {"may", {{"flat", 1}}}};

    document["rules"].push_back(rule);
    document["rules"].push_back(rule);

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheColorsAMapIsDrawnWith)
{
    Map map{.tilemap = defaultTilemap()};

    map.paletteColors[0] = antwika::gfx::Color{
        .red = 9, .green = 8, .blue = 7, .alpha = 255};
    map.paletteColors[3] = antwika::gfx::Color{
        .red = 1, .green = 2, .blue = 3, .alpha = 4};

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(loadedMap.paletteColors, map.paletteColors);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithAPalette)
{
    EXPECT_FALSE(
        holdsAFloat(
            nlohmann::json::parse(
                serializeMap(Map{.tilemap = defaultTilemap()}))));
}

TEST(MapFileTest, ReadMap_GivesAMapWithNoPaletteTheBuiltInOne)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("palette"));
    ageTo(document, 2);

    const auto loadedMap = readText(document.dump());

    EXPECT_EQ(
        loadedMap.paletteColors,
        std::vector<antwika::gfx::Color>(
            antwika::tile::kPaletteColors.begin(),
            antwika::tile::kPaletteColors.end()));
}

TEST(MapFileTest, ReadMap_KeepsThePaletteOfAMapThatCarriedPixels)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));
    nlohmann::json tileset;

    tileset["palette"] = {
        {1, 2, 3, 255}, {4, 5, 6, 255},
        {7, 8, 9, 255}, {10, 11, 12, 255}, {0, 0, 0, 0}};
    tileset["sheets"] = nlohmann::json::array();

    document.erase(std::string("palette"));
    document["tileset"] = tileset;
    ageTo(document, 3);

    const auto loadedMap = readText(document.dump());

    EXPECT_EQ(loadedMap.paletteColors[0].red, 1);
    EXPECT_EQ(loadedMap.paletteColors[3].red, 10);
}

TEST(MapFileTest, ReadMap_RefusesAColorBeyondWhatAColorHolds)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["palette"][0][0] = 300;

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesWhatATileAsksOfItsCorners)
{
    Map map{.tilemap = defaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};

    map.rules.allow(
        tile,
        TileEdge{.side = Side::Top, .edge = EdgeKind::Interior},
        tile);
    map.rules.setCorner(
        tile, antwika::voxel::Corner::TopLeft, false);
    map.rules.setCorner(
        tile, antwika::voxel::Corner::BottomRight, true);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.corner(tile, antwika::voxel::Corner::TopLeft),
        false);
    EXPECT_EQ(
        loadedMap.rules.corner(tile, antwika::voxel::Corner::BottomRight),
        true);
    EXPECT_FALSE(
        loadedMap.rules.corner(tile, antwika::voxel::Corner::TopRight)
            .has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapThatAskedNothingOfCorners)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("corners"));
    ageTo(document, 6);

    const auto loadedMap = readText(document.dump());

    EXPECT_TRUE(
        loadedMap.rules
            .cornersOf(Tile{.atlas = Atlas::Floor, .index = 0})
            .empty());
}

TEST(MapFileTest, WriteMap_CarriesWhereTheCameraStood)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .position = antwika::gfx::Vec3{3.25F, -1.5F, 7.125F},
                .yaw = 0.75F,
                .pitch = -0.5F},
        .zoom = 45};

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_NEAR(loadedMap.camera->transform.position.x, 3.25F, 1e-3F);
    EXPECT_NEAR(loadedMap.camera->transform.position.y, -1.5F, 1e-3F);
    EXPECT_NEAR(loadedMap.camera->transform.position.z, 7.125F, 1e-3F);
    EXPECT_NEAR(loadedMap.camera->transform.yaw, 0.75F, 1e-3F);
    EXPECT_NEAR(loadedMap.camera->transform.pitch, -0.5F, 1e-3F);
    EXPECT_EQ(loadedMap.camera->zoom, 45);
}

[[nodiscard]] antwika::map::Character heroAt(
    const antwika::gfx::Vec3 position, const std::uint8_t way)
{
    return antwika::map::Character{
        .name = "Player",
        .idlePlacement =
            antwika::map::Placement{.position = position, .way = way},
        .player = true};
}

TEST(MapFileTest, WriteMap_CarriesWhereTheCharacterStarts)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {
        heroAt(antwika::gfx::Vec3{3.25F, 6.5F, -2.125F}, 5)};

    const auto loadedMap = readText(serializeMap(map));
    const auto hero = antwika::map::playerIndex(loadedMap);

    ASSERT_TRUE(hero.has_value());

    const auto &stance = loadedMap.characters.at(*hero).idlePlacement;

    EXPECT_NEAR(stance.position.x, 3.25F, 1e-3F);
    EXPECT_NEAR(stance.position.y, 6.5F, 1e-3F);
    EXPECT_NEAR(stance.position.z, -2.125F, 1e-3F);
    EXPECT_EQ(stance.way, 5);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithACharacter)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {
        heroAt(antwika::gfx::Vec3{1.5F, 2.5F, 3.5F}, 3)};

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(serializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoCharacter)
{
    const auto loadedMap =
    readText(serializeMap(Map{.tilemap = defaultTilemap()}));

    EXPECT_FALSE(antwika::map::playerIndex(loadedMap).has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapWrittenBeforeCharactersWereKept)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 19);
    document.erase(std::string("figures"));

    EXPECT_FALSE(
        antwika::map::playerIndex(readText(document.dump()))
            .has_value());
}

TEST(MapFileTest, ReadMap_RefusesACharacterStandingAtAFraction)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {heroAt(antwika::gfx::Vec3{}, 0)};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["characters"][0]["home"]["at"] = {0.5, 0, 0};

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_FacesACharacterKeptBeforeFacingsWereFirst)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 20);
    document.erase(std::string("figures"));
    document["walker"] = {{"at", {1000, 2000, 3000}}};

    const auto loadedMap = readText(document.dump());
    const auto hero = antwika::map::playerIndex(loadedMap);

    ASSERT_TRUE(hero.has_value());
    EXPECT_EQ(loadedMap.characters.at(*hero).idlePlacement.way, 0);
}

TEST(MapFileTest, ReadMap_RefusesACharacterFacingNoWayAtAll)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {heroAt(antwika::gfx::Vec3{}, 0)};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["characters"][0]["home"]["way"] = 8;

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithACamera)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .position = antwika::gfx::Vec3{1.5F, 2.5F, 3.5F},
                .yaw = 0.125F,
                .pitch = -0.875F},
        .zoom = 30};

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(serializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoCamera)
{
    const auto loadedMap =
    readText(serializeMap(Map{.tilemap = defaultTilemap()}));

    EXPECT_FALSE(loadedMap.camera.has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapWrittenBeforeCamerasWereKept)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("camera"));
    ageTo(document, 7);

    EXPECT_FALSE(readText(document.dump()).camera.has_value());
}

TEST(MapFileTest, ReadMap_RefusesACameraStandingAtAFraction)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["camera"] = {{"at", {0, 0, 0}}, {"yaw", 0.5},
                          {"pitch", 0}, {"zoom", 1000}};

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_GivesACameraKeptBeforeZoomTheOpeningOne)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform = antwika::camera::CameraTransform{}, .zoom = 30};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["camera"].erase(std::string("zoom"));
    ageTo(document, 8);

    const auto loadedMap = readText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom, antwika::camera::kDefaultZoom);
}

TEST(MapFileTest, ReadMap_RefusesACameraShowingNothing)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["camera"] = {{"at", {0, 0, 0}}, {"yaw", 0},
                          {"pitch", 0}, {"zoom", 0}};

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsHowTheEditorWasLeftStanding)
{
    Map map{.tilemap = defaultTilemap()};

    map.settings = antwika::map::Settings{
        .lighting = false,
        .showRuleLines = false,
        .tool = antwika::map::Tool::Picker,
        .paint = antwika::map::Paint::Fill,
        .view = antwika::map::View::Atlases};

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(loadedMap.settings, map.settings);
}

TEST(MapFileTest, WriteMap_NamesTheToolsRatherThanNumbersThem)
{
    Map map{.tilemap = defaultTilemap()};

    map.settings.paint = antwika::map::Paint::Line;

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["settings"]["drawing"], "line");
    EXPECT_EQ(document["settings"]["view"], "world");
    EXPECT_EQ(document["settings"]["tool"], "brush");
    EXPECT_TRUE(document["settings"]["lighting"].get<bool>());
}

TEST(MapFileTest, ReadMap_GivesAMapKeptBeforeSettingsTheOpeningOnes)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("settings"));
    ageTo(document, 9);

    const auto loadedMap = readText(document.dump());

    EXPECT_EQ(loadedMap.settings, antwika::map::Settings{});
}

TEST(MapFileTest, ReadMap_RefusesAToolItDoesNotKnow)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["settings"]["tool"] = "chisel";

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsWhatEachVoxelIsMadeOf)
{
    Map map{.tilemap = defaultTilemap()};

    map.voxels = {
        antwika::voxel::VoxelCell{
            .x = 0, .y = 0, .z = 0,
            .kind = antwika::voxel::Kind::Water},
        antwika::voxel::VoxelCell{
            .x = 1, .y = 0, .z = 0,
            .kind = antwika::voxel::Kind::Ramp}};
    map.settings.kind = antwika::voxel::Kind::Ramp;

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 2U);
    EXPECT_EQ(loadedMap.voxels[0].kind, antwika::voxel::Kind::Water);
    EXPECT_EQ(loadedMap.voxels[1].kind, antwika::voxel::Kind::Ramp);
    EXPECT_EQ(loadedMap.settings.kind, antwika::voxel::Kind::Ramp);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["voxels"][0]["kind"], "water");
    EXPECT_EQ(document["settings"]["kind"], "ramp");
}

TEST(MapFileTest, ReadMap_MakesEveryVoxelKeptBeforeTheKindsASolidOne)
{
    Map map{.tilemap = defaultTilemap()};

    map.voxels = {antwika::voxel::VoxelCell{.x = 3, .y = 2, .z = 1}};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["voxels"] = nlohmann::json::array({{3, 2, 1}});
    document["settings"].erase(std::string("kind"));
    ageTo(document, 10);

    const auto loadedMap = readText(document.dump());

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(loadedMap.voxels[0].x, 3);
    EXPECT_EQ(loadedMap.voxels[0].kind, antwika::voxel::Kind::Normal);
    EXPECT_EQ(loadedMap.settings.kind, antwika::voxel::Kind::Normal);
}

TEST(MapFileTest, ReadMap_RefusesAKindItDoesNotKnow)
{
    Map map{.tilemap = defaultTilemap()};

    map.voxels = {antwika::voxel::VoxelCell{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["voxels"][0]["kind"] = "lava";

    EXPECT_THROW((void)readText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsWhichTilesAreGivenToWhichKind)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile poolTile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 4};

    map.rules.setKind(poolTile, antwika::voxel::Kind::Water);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(loadedMap.rules.kindOf(poolTile), antwika::voxel::Kind::Water);
    EXPECT_EQ(
        loadedMap.rules.kindOf(antwika::tilemap::Tile{}),
        antwika::voxel::Kind::Normal);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["tileKinds"][0]["kind"], "water");
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheKindsSolid)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("tileKinds"));
    ageTo(document, 11);

    const auto loadedMap = readText(document.dump());

    EXPECT_TRUE(loadedMap.rules.kinds().empty());
}

TEST(MapFileTest, SidecarPath_NamesTheAtlasAfterItsMap)
{
    EXPECT_EQ(
        antwika::map::sidecarPath("map.json", "atlas-15x9.png"),
        "map-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_KeepsItInTheMapsOwnDirectory)
{
    EXPECT_EQ(
        antwika::map::sidecarPath(
            "worlds/first.json", "atlas-15x12.png"),
        "worlds/first-atlas-15x12.png");
}

TEST(MapFileTest, SidecarPath_TakesAMapWithNoEndingWhole)
{
    EXPECT_EQ(
        antwika::map::sidecarPath("worlds/first", "atlas-15x9.png"),
        "worlds/first-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_LeavesADottedDirectoryAlone)
{
    EXPECT_EQ(
        antwika::map::sidecarPath("a.b/first", "atlas-15x9.png"),
        "a.b/first-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_KeepsTwoMapsApart)
{
    EXPECT_NE(
        antwika::map::sidecarPath("one.json", "atlas-15x9.png"),
        antwika::map::sidecarPath("two.json", "atlas-15x9.png"));
}

TEST(MapFileTest, WriteMap_KeepsWhichWayATileWasDrawnFor)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 115};

    map.rules.setFacing(tile, antwika::voxel::Facing::West);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.facingOf(tile), antwika::voxel::Facing::West);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["tileFacings"][0]["facing"], "west");
}

TEST(MapFileTest, WriteMap_KeepsWhichLevelOfAFlightATileWasDrawnFor)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 160};

    map.rules.setLevel(tile, antwika::voxel::StairHalf::Lower);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.levelOf(tile), antwika::voxel::StairHalf::Lower);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["tileLevels"][0]["level"], "lower");
}

TEST(MapFileTest, WriteMap_KeepsWhichPartOfAFlightATileWasDrawnFor)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 160};

    map.rules.setPart(tile, antwika::voxel::StairPart::Side);

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.partOf(tile), antwika::voxel::StairPart::Side);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["tileParts"][0]["part"], "side");
}

TEST(MapFileTest, WriteMap_KeepsHowStronglyADecorIsWeighed)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::withDecorToggled({}, tile);
    map.decor = antwika::decor::withWeightSet(map.decor, tile, 35);

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().weight, 35);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["decor"][0]["weight"], 35);
}

TEST(MapFileTest, WriteMap_KeepsWhichLayerADecorDressesFor)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::withDecorToggled({}, tile, 2);

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().layer, 2U);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["decor"][0]["layer"], 2);
}

TEST(MapFileTest, ReadMap_GathersDecorKeptBeforeTheLayersOntoTheFirst)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::withDecorToggled({}, tile, 2);

    auto document = nlohmann::json::parse(serializeMap(map));

    document["decor"][0].erase(std::string("layer"));
    ageTo(document, 39);

    const auto loadedMap = readText(document.dump());

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().layer, 1U);
}

TEST(MapFileTest, ReadMap_GivesADecorKeptBeforeTheWeightsFullWeight)
{
    Map map{.tilemap = defaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::withDecorToggled({}, tile);

    auto document = nlohmann::json::parse(serializeMap(map));

    document["decor"][0].erase(std::string("weight"));
    ageTo(document, 38);

    const auto loadedMap = readText(document.dump());

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(
        loadedMap.decor.front().weight,
        antwika::decor::kFullFrequency);
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeThePartsToEitherPart)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("tileParts"));
    ageTo(document, 37);

    const auto loadedMap = readText(document.dump());

    EXPECT_TRUE(loadedMap.rules.parts().empty());
}

TEST(MapFileTest, ReadMap_TakesAZoomKeptAsHowMuchWorldStoodInView)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{.zoom = 45};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 14);
    document["camera"]["zoom"] = 9000;

    const auto loadedMap = readText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom, antwika::camera::kVoxelPixels);
}

TEST(MapFileTest, ReadMap_BringsAZoomBetweenScalesToAWholeOne)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{.zoom = 45};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 14);
    document["camera"]["zoom"] = 3600;

    const auto loadedMap = readText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom % antwika::camera::kVoxelPixels, 0);
}

TEST(MapFileTest, WriteMap_KeepsWhichWayARampWasToldToClimb)
{
    Map map{.tilemap = defaultTilemap()};

    map.voxels.push_back(
        antwika::voxel::VoxelCell{
            .x = 1,
            .y = 0,
            .z = 2,
            .kind = antwika::voxel::Kind::Ramp,
            .facing = antwika::voxel::Facing::North});

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(
        loadedMap.voxels.front().facing, antwika::voxel::Facing::North);

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_EQ(document["voxels"][0]["climb"], "north");
}

TEST(MapFileTest, WriteMap_SaysNothingOfARampToldNothing)
{
    Map map{.tilemap = defaultTilemap()};

    map.voxels.push_back(
        antwika::voxel::VoxelCell{
            .kind = antwika::voxel::Kind::Ramp});

    const auto document = nlohmann::json::parse(serializeMap(map));

    EXPECT_FALSE(document["voxels"][0].contains("climb"));

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(loadedMap.voxels.front().facing, antwika::voxel::Facing::Any);
}

TEST(MapFileTest, ReadMap_MakesASquaredPitchExactAgain)
{
    Map map{.tilemap = defaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .pitch = antwika::camera::isometricPitch()},
        .zoom = 45};

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(
        loadedMap.camera->transform.pitch, antwika::camera::isometricPitch());
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheLevelsToEither)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("tileLevels"));
    ageTo(document, 13);

    const auto loadedMap = readText(document.dump());

    EXPECT_TRUE(loadedMap.rules.levels().empty());
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheFacingsToAnyFlight)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("tileFacings"));
    ageTo(document, 12);

    const auto loadedMap = readText(document.dump());

    EXPECT_TRUE(loadedMap.rules.facings().empty());
}

TEST(MapFileTest, WriteMap_CarriesTheLampsSetDownAboutThePile)
{
    using antwika::light::Lamp;

    Map map{.tilemap = defaultTilemap()};

    map.lamps.push_back(
        Lamp{
            .cell = VoxelCell{.x = 2, .y = 3, .z = -4},
            .tintColor =
                antwika::gfx::Color{
                    .red = 12,
                    .green = 240,
                    .blue = 7,
                    .alpha = 200}});

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.lamps.size(), 1U);
    EXPECT_EQ(loadedMap.lamps.front(), map.lamps.front());
}

TEST(MapFileTest, WriteMap_WritesALampInWholeNumbers)
{
    using antwika::light::Lamp;

    Map map{.tilemap = defaultTilemap()};

    map.lamps.push_back(Lamp{.cell = VoxelCell{.x = -1, .y = 5}});

    EXPECT_FALSE(
        holdsAFloat(nlohmann::json::parse(serializeMap(map))));
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeLampsWithNone)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("lamps"));
    ageTo(document, 16);

    EXPECT_TRUE(readText(document.dump()).lamps.empty());
}

TEST(MapFileTest, ReadMap_RefusesMoreLampsThanTheWorldDrawsBy)
{
    using antwika::light::kMaxLamps;

    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    auto lamps = nlohmann::json::array();

    for (std::size_t index = 0; index <= kMaxLamps; ++index)
    {
        lamps.push_back(
            nlohmann::json{
                {"at", nlohmann::json::array({index, 0, 0})},
                {"tint", nlohmann::json::array({1, 2, 3, 4})}});
    }

    document["lamps"] = lamps;

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeLayersHoldingTheBase)
{
    using antwika::map::kBaseLayer;
    using antwika::map::kBaseLayerName;

    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("layers"));
    ageTo(document, 17);

    const auto layers = readText(document.dump()).layers;

    ASSERT_EQ(layers.size(), 1U);
    EXPECT_EQ(layers.at(kBaseLayer).name, kBaseLayerName);
}

TEST(MapFileTest, WriteMap_CarriesTheLayersAMapIsDrawnIn)
{
    using antwika::map::Layer;

    Map map{.tilemap = defaultTilemap()};

    map.layers.push_back(Layer{.name = "another"});

    EXPECT_EQ(readText(serializeMap(map)).layers, map.layers);
}

TEST(MapFileTest, ReadMap_RefusesMoreLayersThanAMapMayHold)
{
    using antwika::map::kMaxLayers;

    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    auto layers = nlohmann::json::array();

    for (std::size_t index = 0; index <= kMaxLayers; ++index)
    {
        layers.push_back(nlohmann::json{{"name", "one"}});
    }

    document["layers"] = layers;

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            readText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesAPaletteAddedTo)
{
    Map map{.tilemap = defaultTilemap()};

    map.paletteColors.push_back(
        antwika::gfx::Color{
            .red = 1, .green = 2, .blue = 3, .alpha = 4});

    EXPECT_EQ(readText(serializeMap(map)).paletteColors, map.paletteColors);
}

TEST(MapFileTest, WriteMap_CarriesAPaletteTakenFrom)
{
    Map map{.tilemap = defaultTilemap()};

    map.paletteColors.resize(1);

    EXPECT_EQ(readText(serializeMap(map)).paletteColors, map.paletteColors);
}

TEST(MapFileTest, ReadMap_RefusesAPaletteOfNothingAtAll)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["palette"] = nlohmann::json::array();

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            readText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesMoreInksThanAMapMayHold)
{
    using antwika::tile::kMaxInks;

    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    auto colors = nlohmann::json::array();

    for (std::size_t index = 0; index <= kMaxInks; ++index)
    {
        colors.push_back(nlohmann::json::array({1, 2, 3, 4}));
    }

    document["palette"] = colors;

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            readText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, ReadMap_LeavesAMapMixedBeforeWithTheInksItHad)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 18);

    EXPECT_EQ(
        readText(document.dump()).paletteColors.size(),
        antwika::tile::kPaletteSize);
}

TEST(MapFileTest, WriteMap_CarriesTheVariantFamilies)
{
    using antwika::decor::VariantGroup;
    using antwika::decor::VariantMember;

    auto map = demoMap();

    map.familyGroups = {
        VariantGroup{
            .canonicalTile = Tile{.atlas = Atlas::Floor, .index = 1},
            .weight = 50,
            .variants = {
                VariantMember{
                    .tile =
                        Tile{.atlas = Atlas::Floor, .index = 2},
                    .weight = 20},
                VariantMember{
                    .tile = Tile{
                        .atlas = Atlas::Floor, .index = 3}}}}};

    EXPECT_EQ(readText(serializeMap(map)).familyGroups, map.familyGroups);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeFamiliesWithNone)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("families"));
    ageTo(document, 30);

    EXPECT_TRUE(readText(document.dump()).familyGroups.empty());
}

TEST(MapFileTest, ReadMap_RefusesAWeightPastFullFrequency)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["families"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 1})},
            {"weight", 101},
            {"members",
             nlohmann::json::array(
                 {nlohmann::json{
                     {"tile",
                      nlohmann::json::array({"flat", 2})},
                     {"weight", 100}}})}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAGroupOfNoVariantsAtAll)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["families"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 1})},
            {"weight", 100},
            {"members", nlohmann::json::array()}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesASpannedDecorWhole)
{
    using antwika::tilemap::Atlas;

    auto map = demoMap();
    auto decor = antwika::decor::withDecorToggled(
        {}, Tile{.atlas = Atlas::Floor, .index = 3});

    decor = antwika::decor::withSpanSet(
        decor, Tile{.atlas = Atlas::Floor, .index = 3}, 2, 2);
    decor = antwika::decor::withMemberSet(
        decor,
        Tile{.atlas = Atlas::Floor, .index = 3},
        2,
        Tile{.atlas = Atlas::Floor, .index = 5});
    map.decor = decor;

    EXPECT_EQ(readText(serializeMap(map)).decor, map.decor);
}

TEST(MapFileTest, ReadMap_GivesADecorMarkedBeforeSpansOneFace)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.decor = antwika::decor::withDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(serializeMap(map));

    for (auto &entry : document["decor"])
    {
        entry.erase(std::string("span"));
        entry.erase(std::string("members"));
    }

    ageTo(document, 31);

    const auto reloadedMap = readText(document.dump());

    ASSERT_EQ(reloadedMap.decor.size(), 1U);
    EXPECT_EQ(reloadedMap.decor.at(0).width, 1);
    EXPECT_EQ(reloadedMap.decor.at(0).height, 1);
    ASSERT_EQ(reloadedMap.decor.at(0).spanTiles.size(), 1U);
    EXPECT_EQ(
        reloadedMap.decor.at(0).spanTiles.at(0),
        reloadedMap.decor.at(0).tile);
}

TEST(MapFileTest, ReadMap_RefusesASpanTooWideToStamp)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.decor = antwika::decor::withDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(serializeMap(map));

    document["decor"][0]["span"] =
        nlohmann::json::array({5, 1});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesSpanTilesThatDoNotFillTheSpan)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.decor = antwika::decor::withDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(serializeMap(map));

    document["decor"][0]["span"] =
        nlohmann::json::array({2, 2});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheFlipsOfItsTiles)
{
    using antwika::tilemap::Atlas;

    auto map = demoMap();
    auto flips = antwika::decor::withAnimationToggled(
        {}, Tile{.atlas = Atlas::Floor, .index = 3});

    flips = antwika::decor::withAnimationFrameAdded(
        flips, Tile{.atlas = Atlas::Floor, .index = 3});
    flips = antwika::decor::withAnimationFrameSet(
        flips,
        Tile{.atlas = Atlas::Floor, .index = 3},
        1,
        Tile{.atlas = Atlas::Floor, .index = 9});
    map.flipAnimations = flips;

    EXPECT_EQ(readText(serializeMap(map)).flipAnimations, map.flipAnimations);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeFlipsStill)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("flips"));
    ageTo(document, 32);

    EXPECT_TRUE(readText(document.dump()).flipAnimations.empty());
}

TEST(MapFileTest, ReadMap_RefusesAFlipOfNineFrames)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    auto frames = nlohmann::json::array();

    for (std::size_t index = 0; index < 9; ++index)
    {
        frames.push_back(nlohmann::json::array({"flat", index}));
    }

    frames[0] = nlohmann::json::array({"flat", 0});
    document["flips"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 0})},
            {"frames", frames}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAFlipAcrossTheAtlases)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["flips"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 0})},
            {"frames",
             nlohmann::json::array(
                 {nlohmann::json::array({"flat", 0}),
                  nlohmann::json::array({"upright", 1})})}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheTransitionsItWove)
{
    using antwika::tilemap::Atlas;

    auto map = demoMap();

    map.transitions = {
        antwika::tile::TransitionTile{
            .fromTile = Tile{.atlas = Atlas::Floor, .index = 1},
            .toTile = Tile{.atlas = Atlas::Floor, .index = 2},
            .maskTile = Tile{.atlas = Atlas::Floor, .index = 8},
            .outputTile = Tile{.atlas = Atlas::Floor, .index = 9}}};

    EXPECT_EQ(
        readText(serializeMap(map)).transitions, map.transitions);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeTransitionsBare)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("transitions"));
    ageTo(document, 33);

    EXPECT_TRUE(readText(document.dump()).transitions.empty());
}

TEST(MapFileTest, ReadMap_RefusesATransitionAcrossAtlases)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["transitions"] = nlohmann::json::array(
        {nlohmann::json{
            {"from", nlohmann::json::array({"upright", 1})},
            {"to", nlohmann::json::array({"flat", 2})},
            {"mask", nlohmann::json::array({"flat", 8})},
            {"slot", nlohmann::json::array({"flat", 9})}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesATransitionIntoItself)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["transitions"] = nlohmann::json::array(
        {nlohmann::json{
            {"from", nlohmann::json::array({"flat", 1})},
            {"to", nlohmann::json::array({"flat", 1})},
            {"mask", nlohmann::json::array({"flat", 8})},
            {"slot", nlohmann::json::array({"flat", 9})}}});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheGatesItHolds)
{
    auto map = demoMap();

    map.keyCells = {VoxelCell{.x = 1, .y = 0, .z = 2}};
    map.doorCells = {
        VoxelCell{.x = 3, .y = 0, .z = 2},
        VoxelCell{.x = 3, .y = 1, .z = 2}};
    map.checkpointCells = {VoxelCell{.x = 5, .y = 0, .z = 5}};
    map.exitLocked = true;

    const auto reloadedMap = readText(serializeMap(map));

    EXPECT_EQ(reloadedMap.keyCells, map.keyCells);
    EXPECT_EQ(reloadedMap.doorCells, map.doorCells);
    EXPECT_EQ(reloadedMap.checkpointCells, map.checkpointCells);
    EXPECT_TRUE(reloadedMap.exitLocked);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeGatesBare)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document.erase(std::string("keys"));
    document.erase(std::string("doors"));
    document.erase(std::string("checkpoints"));
    document.erase(std::string("exitLocked"));
    ageTo(document, 34);

    const auto reloadedMap = readText(document.dump());

    EXPECT_TRUE(reloadedMap.keyCells.empty());
    EXPECT_TRUE(reloadedMap.doorCells.empty());
    EXPECT_TRUE(reloadedMap.checkpointCells.empty());
    EXPECT_FALSE(reloadedMap.exitLocked);
}

TEST(MapFileTest, WriteMap_CarriesTheItemsItHolds)
{
    auto map = demoMap();

    map.foodCells = {VoxelCell{.x = 1, .y = 0, .z = 2}};
    map.waterCells = {
        VoxelCell{.x = 3, .y = 0, .z = 2},
        VoxelCell{.x = 3, .y = 1, .z = 2}};

    const auto reloadedMap = readText(serializeMap(map));

    EXPECT_EQ(reloadedMap.foodCells, map.foodCells);
    EXPECT_EQ(reloadedMap.waterCells, map.waterCells);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeItemsBare)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 41);

    const auto reloadedMap = readText(document.dump());

    EXPECT_TRUE(reloadedMap.foodCells.empty());
    EXPECT_TRUE(reloadedMap.waterCells.empty());
}

TEST(MapFileTest, ReadMap_RefusesAnItemBeyondTheLattice)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["water"] = nlohmann::json::array(
        {nlohmann::json::array({kMaxCellCoord + 1, 0, 0})});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheChosenGateTool)
{
    auto map = demoMap();

    map.settings.tool = antwika::map::Tool::Checkpoint;

    EXPECT_EQ(
        readText(serializeMap(map)).settings.tool,
        antwika::map::Tool::Checkpoint);
}

TEST(MapFileTest, ReadMap_RefusesAKeyBeyondTheLattice)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    document["keys"] = nlohmann::json::array(
        {nlohmann::json::array(
            {kMaxCellCoord + 1, 0, 0})});

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

[[nodiscard]] bool carries(
    const antwika::map::Character &character,
    const std::string_view name)
{
    return std::ranges::find(character.components, name)
           != character.components.end();
}

TEST(MapFileTest, WriteMap_CarriesTheComponentsAFigureNames)
{
    auto map = demoMap();

    map.characters = {
        antwika::map::Character{.components = {"component::CarriedLight"}}};

    EXPECT_TRUE(carries(
        readText(serializeMap(map)).characters.at(0),
        "component::CarriedLight"));
}

TEST(MapFileTest, ReadMap_LeavesAFigureDrawnBeforeLampsCarryingNothing)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 35);

    for (auto &figure : document["figures"])
    {
        figure.erase(std::string("lamp"));
    }

    EXPECT_FALSE(carries(
        readText(document.dump()).characters.at(0),
        "component::CarriedLight"));
}

TEST(MapFileTest, LoadMap_TakesEveryMapTheRepositoryShips)
{
    const std::array<std::string_view, 2> shippedPaths{
        "assets/maps/map.json", "assets/maps/occlusion.json"};

    for (const auto path : shippedPaths)
    {
        std::ifstream inputStream{std::string(path)};

        if (!inputStream)
        {
            continue;
        }

        const std::string text{
            std::istreambuf_iterator<char>(inputStream),
            std::istreambuf_iterator<char>()};

        const auto loadedMap = readText(text);
        const auto hero = antwika::map::playerIndex(loadedMap);

        ASSERT_TRUE(hero.has_value());
        EXPECT_TRUE(carries(
            loadedMap.characters.at(*hero), "component::CarriedLight"));
        EXPECT_TRUE(carries(
            loadedMap.characters.at(*hero), "component::Position"));
        EXPECT_TRUE(carries(
            loadedMap.characters.at(*hero), "component::Player"));
    }
}

TEST(MapFileTest, ReadMap_TurnsAnOlderLampFlagIntoACarriedLight)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 42);

    for (auto &figure : document["characters"])
    {
        figure["lamp"] = true;
    }

    EXPECT_TRUE(carries(
        readText(document.dump()).characters.at(0),
        "component::CarriedLight"));
}

TEST(MapFileTest, ReadMap_MovesOlderComponentNamesIntoComponent)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 44);

    for (auto &figure : document["characters"])
    {
        figure["components"] = nlohmann::json::array(
            {"collision::Position",
             "collision::Velocity",
             "collision::Player",
             "character::AnimationState",
             "character::RosterIndex",
             "character::Speaker",
             "light::CarriedLight",
             "light::FillLight"});
    }

    const auto loadedCharacter =
        readText(document.dump()).characters.at(0);

    EXPECT_TRUE(carries(loadedCharacter, "component::Position"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Velocity"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Player"));
    EXPECT_TRUE(carries(loadedCharacter, "component::AnimationState"));
    EXPECT_TRUE(carries(loadedCharacter, "component::RosterIndex"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Speaker"));
    EXPECT_TRUE(carries(loadedCharacter, "component::CarriedLight"));
    EXPECT_TRUE(carries(loadedCharacter, "component::FillLight"));
}

TEST(MapFileTest, ReadMap_LeavesAComponentItDoesNotRenameAlone)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 44);

    for (auto &figure : document["characters"])
    {
        figure["components"] =
            nlohmann::json::array({"component::Health"});
    }

    EXPECT_TRUE(carries(
        readText(document.dump()).characters.at(0),
        "component::Health"));
}

TEST(MapFileTest, ReadMap_RefusesFigureComponentsOfNumbers)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["characters"][0]["components"] = {3};

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_LetsGoOfWhatADecorWasCalled)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.decor = antwika::decor::withDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(serializeMap(map));

    document["decor"][0]["name"] = "moss";
    ageTo(document, 36);

    EXPECT_EQ(readText(document.dump()).decor, map.decor);
}

TEST(MapFileTest, SaveMap_KeepsWhatItWroteOverUnderTheBackupName)
{
    const auto path = somewhereToWrite("antwika-backup-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);
    auto first = demoMap();

    first.ambient = 11;

    auto second = first;

    second.ambient = 77;

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    saveMap(path, first);

    EXPECT_FALSE(std::filesystem::exists(backupPath));

    saveMap(path, second);

    ASSERT_TRUE(std::filesystem::exists(backupPath));
    EXPECT_EQ(loadMap(path), second);
    EXPECT_EQ(loadMap(backupPath), first);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SaveMap_LeavesNothingLyingHalfWritten)
{
    const auto path = somewhereToWrite("antwika-halfway-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);
    const auto writingPath =
        path + std::string(antwika::map::kWritingSuffix);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    saveMap(path, demoMap());

    EXPECT_FALSE(std::filesystem::exists(writingPath));

    saveMap(path, demoMap());

    EXPECT_FALSE(std::filesystem::exists(writingPath));

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SaveMap_KeepsOnlyTheLastMapItWroteOver)
{
    const auto path = somewhereToWrite("antwika-onebak-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    for (const std::int32_t ambient : {1, 2, 3})
    {
        auto map = demoMap();

        map.ambient = ambient;
        saveMap(path, map);
    }

    EXPECT_EQ(loadMap(path).ambient, 3);
    EXPECT_EQ(loadMap(backupPath).ambient, 2);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SharedTexturePath_PutsItBesideTheMapsOwnFolder)
{
    EXPECT_EQ(
        antwika::map::sharedTexturePath(
            "assets/maps/map.json", "character-20x28.png"),
        "assets/textures/character-20x28.png");
}

TEST(MapFileTest, SharedTexturePath_IsTheSameForEveryMapOfAFolder)
{
    EXPECT_EQ(
        antwika::map::sharedTexturePath(
            "assets/maps/one.json", "icons-16.png"),
        antwika::map::sharedTexturePath(
            "assets/maps/another.json", "icons-16.png"));
}

TEST(MapFileTest, SharedTexturePath_KeepsTheNameWhole)
{
    EXPECT_EQ(
        std::filesystem::path(
            antwika::map::sharedTexturePath(
                "assets/maps/map.json", "icons-16.png"))
            .filename()
            .string(),
        "icons-16.png");
}

TEST(MapFileTest, SharedTexturePath_IsNotBesideTheMapItself)
{
    const std::string mapPath = "assets/maps/map.json";

    EXPECT_NE(
        antwika::map::sharedTexturePath(
            mapPath, "character-20x28.png"),
        antwika::map::sidecarPath(mapPath, "character-20x28.png"));
}

TEST(MapFileTest, WriteMap_KeepsTheRosterOfCharacters)
{
    auto map = demoMap();

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel"}};

    const auto loadedMap = readText(serializeMap(map));

    ASSERT_EQ(loadedMap.characters.size(), 2U);
    EXPECT_EQ(loadedMap.characters.at(0).name, "Ada");
    EXPECT_EQ(loadedMap.characters.at(1).name, "Bel");
}

TEST(MapFileTest, WriteMap_MarksThePlayerAmongTheCharacters)
{
    auto map = demoMap();

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel", .player = true}};

    const auto loadedMap = readText(serializeMap(map));

    EXPECT_EQ(
        antwika::map::playerIndex(loadedMap),
        std::optional<std::size_t>{1});
}

TEST(MapFileTest, WriteMap_NoLongerWritesAWalkerBesideTheRoster)
{
    const auto document = nlohmann::json::parse(
        serializeMap(demoMap()));

    EXPECT_FALSE(document.contains("walker"));
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithTwoPlayers)
{
    auto map = demoMap();

    map.characters = {
        antwika::map::Character{.player = true},
        antwika::map::Character{.player = true}};

    EXPECT_THROW(
        (void)readText(serializeMap(map)),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_MakesAPlayerOfTheWalkerOfAnOlderMap)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 40);
    document["walker"] = {{"at", {1000, 2000, 3000}}, {"way", 3}};

    const auto loadedMap = readText(document.dump());
    const auto hero = antwika::map::playerIndex(loadedMap);

    ASSERT_TRUE(hero.has_value());

    const auto &stance = loadedMap.characters.at(*hero).idlePlacement;

    EXPECT_NEAR(stance.position.x, 1.0F, 1e-3F);
    EXPECT_NEAR(stance.position.y, 2.0F, 1e-3F);
    EXPECT_NEAR(stance.position.z, 3.0F, 1e-3F);
    EXPECT_TRUE(carries(
        loadedMap.characters.at(*hero), "component::CarriedLight"));
}

TEST(MapFileTest, ReadMap_LeavesAnOlderMapWithNoWalkerWithNoPlayer)
{
    auto document = nlohmann::json::parse(
        serializeMap(Map{.tilemap = defaultTilemap()}));

    ageTo(document, 40);

    EXPECT_FALSE(
        antwika::map::playerIndex(readText(document.dump()))
            .has_value());
}

TEST(MapFileTest, ReadMap_KeepsTheFiguresOfAnOlderMapBeforeThePlayer)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel"}};

    auto document = nlohmann::json::parse(serializeMap(map));

    ageTo(document, 40);
    document["walker"] = {{"at", {0, 0, 0}}, {"way", 0}};

    const auto loadedMap = readText(document.dump());

    ASSERT_EQ(loadedMap.characters.size(), 3U);
    EXPECT_EQ(loadedMap.characters.at(0).name, "Ada");
    EXPECT_EQ(loadedMap.characters.at(1).name, "Bel");
    EXPECT_EQ(
        antwika::map::playerIndex(loadedMap),
        std::optional<std::size_t>{2});
}

TEST(MapFileTest, ReadMap_TurnsAFigureOfAnOlderMapTheWayItStood)
{
    const std::vector<std::pair<int, std::uint8_t>> turns{
        {0, 2}, {1, 4}, {2, 0}, {3, 6}};

    for (const auto &[was, becomes] : turns)
    {
        auto map = Map{.tilemap = defaultTilemap()};

        map.characters = {antwika::map::Character{}};

        auto document = nlohmann::json::parse(serializeMap(map));

        ageTo(document, 40);
        document["figures"][0]["home"]["way"] = was;

        EXPECT_EQ(
            readText(document.dump())
                .characters.at(0)
                .idlePlacement.way,
            becomes);
    }
}

TEST(MapFileTest, ReadMap_RefusesACharacterPlayerFlagOfNumbers)
{
    auto map = Map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(serializeMap(map));

    document["characters"][0]["player"] = 3;

    EXPECT_THROW(
        (void)readText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, PlayerIndex_FindsTheOnePlayerOfARoster)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {
        antwika::map::Character{},
        antwika::map::Character{.player = true}};

    EXPECT_EQ(
        antwika::map::playerIndex(map),
        std::optional<std::size_t>{1});
}

TEST(MapFileTest, PlayerIndex_GivesNothingWhereNoneIsMarked)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {antwika::map::Character{}};

    EXPECT_FALSE(antwika::map::playerIndex(map).has_value());
}

TEST(MapFileTest, PatrolStopsOf_GivesEveryCharacterItsStopsInOrder)
{
    Map map{.tilemap = defaultTilemap()};

    map.characters = {
        antwika::map::Character{
            .patrolPathCells = {VoxelCell{.x = 1, .y = 2, .z = 3}}},
        antwika::map::Character{}};

    const auto stops = antwika::map::patrolStopsOf(map);

    ASSERT_EQ(stops.size(), 2U);
    ASSERT_EQ(stops.at(0).size(), 1U);
    EXPECT_EQ(stops.at(0).at(0).x, 1);
    EXPECT_TRUE(stops.at(1).empty());
}
