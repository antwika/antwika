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

#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/loadout/ComponentValue.hpp>
#include <antwika/voxelmap/Voxel.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>

#include <antwika/map/MapFile.hpp>
#include <antwika/map/MapFileError.hpp>
#include <antwika/enums/Enumeration.hpp>

using antwika::tilemap::Atlas;
using antwika::tilemap::getDefaultTilemap;
using antwika::voxel::EdgeKind;
using antwika::voxel::voxelsOf;
using antwika::tilemap::kEveryTileEdge;
using antwika::map::kMaxCellCoord;
using antwika::map::getLoadMap;
using antwika::map::Map;
using antwika::map::MapFileError;
using antwika::map::getSerializeMap;
using antwika::voxelmap::getDemoCells;
using antwika::map::readMap;
using antwika::map::saveMap;
using antwika::voxel::Side;
using antwika::tilemap::swapTiles;
using antwika::tilemap::Tile;
using antwika::tilemap::TileEdge;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;

namespace
{
    [[nodiscard]] Map getReadText(const std::string &text)
    {
        std::istringstream inputStream(text);

        return readMap(inputStream);
    }

    void ageTo(
        nlohmann::json &document, const std::uint32_t version)
    {
        if (version < 51)
        {
            for (auto &figure : document["characters"])
            {
                auto &names = figure[std::string("components")];

                for (auto &name : names)
                {
                    if (name.get<std::string>()
                        == "component::CharacterIndex")
                    {
                        name = std::string("component::RosterIndex");
                    }
                }

                auto &values =
                    figure[std::string("componentValues")];

                if (values.contains("component::CharacterIndex"))
                {
                    values["component::RosterIndex"] =
                        values["component::CharacterIndex"];
                    values.erase(
                        std::string("component::CharacterIndex"));
                }
            }
        }

        if (version < 50)
        {
            for (auto &figure : document["characters"])
            {
                figure["tuning"] = figure["componentValues"];
                figure.erase(std::string("componentValues"));
            }
        }

        if (version < 49)
        {
            document["keys"] = nlohmann::json::array();
            document["doors"] = nlohmann::json::array();
            document["plates"] = nlohmann::json::array();
            document["exitLocked"] = false;
        }

        if (version < 48)
        {
            for (auto &figure : document["characters"])
            {
                figure.erase(std::string("tuning"));
            }
        }

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

    [[nodiscard]] Map getDemoMap()
    {
        return Map{
            .voxels = getDemoCells(), .tilemap = getDefaultTilemap()};
    }

    [[nodiscard]] std::string getAroundTiles(const std::string &tiles)
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

    [[nodiscard]] std::string getSomewhereToWrite(
        const std::string &name)
    {
        const auto path =
            std::filesystem::temp_directory_path() / name;

        return path.string();
    }
}

TEST(MapFileTest, Map_ReadsBackTheMapItWrote)
{
    const auto map = getDemoMap();

    EXPECT_EQ(getReadText(getSerializeMap(map)), map);
}

TEST(MapFileTest, Map_ReadsBackAMapHoldingNothingAtAll)
{
    const Map bareMap;

    EXPECT_EQ(getReadText(getSerializeMap(bareMap)), bareMap);
}

TEST(MapFileTest, Map_KeepsHowTheTilesHaveBeenArranged)
{
    auto map = getDemoMap();

    swapTiles(map.tilemap, {.column = 0, .row = 0},
              {.column = 31, .row = 15});
    swapTiles(map.tilemap, {.column = 4, .row = 2},
              {.column = 5, .row = 2});

    const auto reloadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(reloadedMap, map);
    EXPECT_EQ(reloadedMap.tilemap.getEntryAt(0, 0), *map.tilemap.getEntryAt(0, 0));
    EXPECT_NE(reloadedMap.tilemap.getEntryAt(0, 0), getDefaultTilemap().getEntryAt(0, 0));
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
    map.spawnCubePosition = VoxelPosition{.x = 2, .y = 0, .z = -2};
    map.exitCubePosition = VoxelPosition{.x = -4, .y = 2, .z = 6};

    const auto reloadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(reloadedMap, map);
    EXPECT_EQ(reloadedMap.decor, map.decor);
    EXPECT_EQ(reloadedMap.decorRules, map.decorRules);
    EXPECT_EQ(reloadedMap.spawnCubePosition, map.spawnCubePosition);
    EXPECT_EQ(reloadedMap.exitCubePosition, map.exitCubePosition);
}

TEST(MapFileTest, Map_KeepsThePlacesItWasGiven)
{
    Map map;

    map.voxels = voxelsOf({
        VoxelCell{.position = {.x = -kMaxCellCoord, .y = 0,
            .z = kMaxCellCoord}},
        VoxelCell{.position = {.x = 7, .y = -3, .z = 11}}});

    EXPECT_EQ(getReadText(getSerializeMap(map)).voxels, map.voxels);
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
    const auto documentJson = nlohmann::json::parse(getSerializeMap(getDemoMap()));

    EXPECT_FALSE(holdsAFloat(documentJson));
}

TEST(MapFileTest, Map_SaysWhatItIsAndWhichVersionItIs)
{
    const auto text = getSerializeMap(Map{});

    EXPECT_NE(text.find("antwika.map"), std::string::npos);
    EXPECT_NE(text.find("\"version\""), std::string::npos);
}

TEST(MapFileTest, ReadMap_TurnsAwayTextThatIsNotJson)
{
    EXPECT_THROW((void)getReadText("this is not json"), MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAFileOfAnotherKind)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.voxels", "version": 1,
                "voxels": [], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAVersionItDoesNotKnow)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 99,
                "voxels": [], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithNoTilemap)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": []})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithNoVoxels)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 1,
                "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAPlaceOfTheWrongLength)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [[1, 2]], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayTwoVoxelsStandingInOnePlace)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [[1, 2, 3], [1, 2, 3]], "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TakesTwoVoxelsThatStandApart)
{
    const auto loadedMap = getReadText(
        R"({"magic": "antwika.map", "version": 1,
            "voxels": [[1, 2, 3], [1, 2, 4]], "tilemap":
            {"columns": 0, "rows": 0, "tiles": []}})");

    EXPECT_EQ(loadedMap.voxels.size(), 2U);
}

TEST(MapFileTest, ReadMap_TurnsAwayAPlaceThatIsNotAWholeNumber)
{
    EXPECT_THROW(
        (void)getReadText(
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
        (void)getReadText(
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
        (void)getReadText(
            getAroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["sideways", 0]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayATileOfATilesThereIsNot)
{
    EXPECT_THROW(
        (void)getReadText(
            getAroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["flat", 256]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayATileOfTheWrongLength)
{
    EXPECT_THROW(
        (void)getReadText(
            getAroundTiles(
                R"({"columns": 1, "rows": 1,
                    "tiles": [["flat"]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TurnsAwayAGridItHoldsTooFewTilesFor)
{
    EXPECT_THROW(
        (void)getReadText(
            getAroundTiles(
                R"({"columns": 8, "rows": 8,
                    "tiles": [["flat", 0]]})")),
        MapFileError);
}

TEST(MapFileTest, ReadMap_TakesAGridItHoldsEveryTileFor)
{
    const auto map = getReadText(
        getAroundTiles(
            R"({"columns": 2, "rows": 1,
                "tiles": [["flat", 3], ["upright", 4]]})"));

    ASSERT_TRUE(map.tilemap.isComplete());
    EXPECT_EQ(map.tilemap.getEntryAt(0, 0)->index, 3U);
    EXPECT_EQ(map.tilemap.getEntryAt(1, 0)->index, 4U);
}

TEST(MapFileTest, ReadMap_TurnsAwayAMemberItDoesNotKnow)
{
    EXPECT_THROW(
        (void)getReadText(
            R"({"magic": "antwika.map", "version": 1,
                "voxels": [], "surprise": 1, "tilemap":
                {"columns": 0, "rows": 0, "tiles": []}})"),
        MapFileError);
}

TEST(MapFileTest, SaveMap_WritesAMapLoadMapReadsBack)
{
    const auto path = getSomewhereToWrite("antwika-map.json");
    const auto map = getDemoMap();

    saveMap(path, map);

    EXPECT_EQ(getLoadMap(path), map);

    std::filesystem::remove(path);
}

TEST(MapFileTest, SaveMap_CarriesTheRulesThroughAFile)
{
    const auto path = getSomewhereToWrite("antwika-ruled-map.json");
    auto map = getDemoMap();

    map.rules.allow(
        *map.tilemap.getEntryAt(2, 0),
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary},
        *map.tilemap.getEntryAt(5, 3));
    map.rules.allow(
        *map.tilemap.getEntryAt(2, 0),
        TileEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary},
        *map.tilemap.getEntryAt(6, 3));

    saveMap(path, map);

    const auto loadedMap = getLoadMap(path);

    EXPECT_EQ(loadedMap, map);
    EXPECT_EQ(loadedMap.rules, map.rules);
    EXPECT_EQ(loadedMap.rules.getSize(), 2U);

    std::filesystem::remove(path);
}

TEST(MapFileTest, LoadMap_SaysSoWhenThereIsNoSuchFile)
{
    EXPECT_THROW(
        (void)getLoadMap(getSomewhereToWrite("antwika-no-such-map.json")),
        MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheRulesThroughAWrittenMap)
{
    Map map{.voxels = getDemoCells(), .tilemap = getDefaultTilemap()};
    constexpr Tile wallTile{.atlas = Atlas::Wall, .index = 7};
    constexpr TileEdge aboveEdge{
        .side = Side::Top, .edge = EdgeKind::Boundary};

    map.rules.allow(*map.tilemap.getEntryAt(0, 0), aboveEdge, wallTile);
    map.rules.allow(*map.tilemap.getEntryAt(0, 0), aboveEdge, *map.tilemap.getEntryAt(1, 0));
    map.rules.allow(
        *map.tilemap.getEntryAt(4, 2),
        TileEdge{.side = Side::Right, .edge = EdgeKind::Interior},
        wallTile);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(loadedMap.rules, map.rules);
    EXPECT_EQ(loadedMap.rules.getSize(), 3U);
    EXPECT_TRUE(
        loadedMap.rules.allows(*map.tilemap.getEntryAt(0, 0), aboveEdge, wallTile));
}

TEST(MapFileTest, WriteMap_KeepsTheRulesOfEveryEdgeApart)
{
    Map map{.tilemap = getDefaultTilemap()};
    constexpr Tile oneTile{.atlas = Atlas::Wall, .index = 1};
    constexpr Tile otherTile{.atlas = Atlas::Floor, .index = 2};

    for (const auto edge : kEveryTileEdge)
    {
        map.rules.allow(oneTile, edge, otherTile);
    }

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(loadedMap.rules, map.rules);

    for (const auto edge : kEveryTileEdge)
    {
        EXPECT_TRUE(loadedMap.rules.allows(oneTile, edge, otherTile));
    }
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenOnceRulesAreKept)
{
    Map map{.voxels = getDemoCells(), .tilemap = getDefaultTilemap()};

    map.rules.allow(
        *map.tilemap.getEntryAt(3, 1),
        TileEdge{.side = Side::Left, .edge = EdgeKind::Interior},
        *map.tilemap.getEntryAt(9, 4));

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(getSerializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoRulesYet)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("rules"));
    ageTo(document, 1);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_EQ(loadedMap.rules.getSize(), 0U);
    EXPECT_EQ(loadedMap.tilemap, getDefaultTilemap());
}

TEST(MapFileTest, ReadMap_RefusesARuleForAnEdgeThatIsNot)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["rules"].push_back(
        {{"tile", {"upright", 0}},
         {"side", "sideways"},
         {"edge", "outward"},
         {"may", {{"flat", 1}}}});

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesAnEdgeShutAgainstEverything)
{
    Map map{.tilemap = getDefaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};
    constexpr TileEdge shutEdge{
        .side = Side::Left, .edge = EdgeKind::Interior};
    constexpr TileEdge openEdge{
        .side = Side::Right, .edge = EdgeKind::Interior};

    map.rules.forbidAll(tile, shutEdge);
    map.rules.allow(tile, openEdge, tile);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_TRUE(loadedMap.rules.isForbidden(tile, shutEdge));
    EXPECT_FALSE(loadedMap.rules.hasNoRule(tile, shutEdge));
    EXPECT_FALSE(loadedMap.rules.isForbidden(tile, openEdge));
    EXPECT_TRUE(loadedMap.rules.allows(tile, openEdge, tile));
    EXPECT_EQ(loadedMap.rules, map.rules);
}

TEST(MapFileTest, ReadMap_TellsAShutEdgeFromOneNeverSpokenOf)
{
    Map map{.tilemap = getDefaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};
    constexpr TileEdge shutEdge{
        .side = Side::Left, .edge = EdgeKind::Interior};
    constexpr TileEdge neverEdge{
        .side = Side::Top, .edge = EdgeKind::Interior};

    map.rules.forbidAll(tile, shutEdge);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_TRUE(loadedMap.rules.isForbidden(tile, shutEdge));
    EXPECT_TRUE(loadedMap.rules.hasNoRule(tile, neverEdge));
    EXPECT_FALSE(loadedMap.rules.isForbidden(tile, neverEdge));
}

TEST(MapFileTest, ReadMap_RefusesTheSameNeighbourAllowedTwice)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["rules"].push_back(
        {{"tile", {"upright", 0}},
         {"side", "top"},
         {"edge", "outward"},
         {"may", {{"flat", 1}, {"flat", 1}}}});

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesOneEdgeOfOneTileSpokenOfTwice)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));
    const nlohmann::json rule = {
        {"tile", {"upright", 0}},
        {"side", "top"},
        {"edge", "outward"},
        {"may", {{"flat", 1}}}};

    document["rules"].push_back(rule);
    document["rules"].push_back(rule);

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheColorsAMapIsDrawnWith)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.paletteColors[0] = antwika::gfx::Color{
        .red = 9, .green = 8, .blue = 7, .alpha = 255};
    map.paletteColors[3] = antwika::gfx::Color{
        .red = 1, .green = 2, .blue = 3, .alpha = 4};

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(loadedMap.paletteColors, map.paletteColors);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithAPalette)
{
    EXPECT_FALSE(
        holdsAFloat(
            nlohmann::json::parse(
                getSerializeMap(Map{.tilemap = getDefaultTilemap()}))));
}

TEST(MapFileTest, ReadMap_GivesAMapWithNoPaletteTheBuiltInOne)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("palette"));
    ageTo(document, 2);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_EQ(
        loadedMap.paletteColors,
        std::vector<antwika::gfx::Color>(
            antwika::tile::kPaletteColors.begin(),
            antwika::tile::kPaletteColors.end()));
}

TEST(MapFileTest, ReadMap_KeepsThePaletteOfAMapThatCarriedPixels)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));
    nlohmann::json tileset;

    tileset["palette"] = {
        {1, 2, 3, 255}, {4, 5, 6, 255},
        {7, 8, 9, 255}, {10, 11, 12, 255}, {0, 0, 0, 0}};
    tileset["sheets"] = nlohmann::json::array();

    document.erase(std::string("palette"));
    document["tileset"] = tileset;
    ageTo(document, 3);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_EQ(loadedMap.paletteColors[0].red, 1);
    EXPECT_EQ(loadedMap.paletteColors[3].red, 10);
}

TEST(MapFileTest, ReadMap_RefusesAColorBeyondWhatAColorHolds)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["palette"][0][0] = 300;

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesWhatATileAsksOfItsCorners)
{
    Map map{.tilemap = getDefaultTilemap()};
    constexpr Tile tile{.atlas = Atlas::Floor, .index = 3};

    map.rules.allow(
        tile,
        TileEdge{.side = Side::Top, .edge = EdgeKind::Interior},
        tile);
    map.rules.setCorner(
        tile, antwika::voxel::Corner::TopLeft, false);
    map.rules.setCorner(
        tile, antwika::voxel::Corner::BottomRight, true);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.getCorner(tile, antwika::voxel::Corner::TopLeft),
        false);
    EXPECT_EQ(
        loadedMap.rules.getCorner(tile, antwika::voxel::Corner::BottomRight),
        true);
    EXPECT_FALSE(
        loadedMap.rules.getCorner(tile, antwika::voxel::Corner::TopRight)
            .has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapThatAskedNothingOfCorners)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("corners"));
    ageTo(document, 6);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_TRUE(
        loadedMap.rules
            .cornersOf(Tile{.atlas = Atlas::Floor, .index = 0})
            .empty());
}

TEST(MapFileTest, WriteMap_CarriesWhereTheCameraStood)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .position = antwika::gfx::Vec3{3.25F, -1.5F, 7.125F},
                .yaw = 0.75F,
                .pitch = -0.5F},
        .zoom = 45};

    const auto loadedMap = getReadText(getSerializeMap(map));

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
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {
        heroAt(antwika::gfx::Vec3{3.25F, 6.5F, -2.125F}, 5)};

    const auto loadedMap = getReadText(getSerializeMap(map));
    const auto hero = antwika::map::getPlayerIndex(loadedMap);

    ASSERT_TRUE(hero.has_value());

    const auto &stance = loadedMap.characters.at(*hero).idlePlacement;

    EXPECT_NEAR(stance.position.x, 3.25F, 1e-3F);
    EXPECT_NEAR(stance.position.y, 6.5F, 1e-3F);
    EXPECT_NEAR(stance.position.z, -2.125F, 1e-3F);
    EXPECT_EQ(stance.way, 5);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithACharacter)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {
        heroAt(antwika::gfx::Vec3{1.5F, 2.5F, 3.5F}, 3)};

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(getSerializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoCharacter)
{
    const auto loadedMap =
    getReadText(getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    EXPECT_FALSE(antwika::map::getPlayerIndex(loadedMap).has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapWrittenBeforeCharactersWereKept)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 19);
    document.erase(std::string("figures"));

    EXPECT_FALSE(
        antwika::map::getPlayerIndex(getReadText(document.dump()))
            .has_value());
}

TEST(MapFileTest, ReadMap_RefusesACharacterStandingAtAFraction)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {heroAt(antwika::gfx::Vec3{}, 0)};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["home"]["at"] = {0.5, 0, 0};

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_FacesACharacterKeptBeforeFacingsWereFirst)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 20);
    document.erase(std::string("figures"));
    document["walker"] = {{"at", {1000, 2000, 3000}}};

    const auto loadedMap = getReadText(document.dump());
    const auto hero = antwika::map::getPlayerIndex(loadedMap);

    ASSERT_TRUE(hero.has_value());
    EXPECT_EQ(loadedMap.characters.at(*hero).idlePlacement.way, 0);
}

TEST(MapFileTest, ReadMap_RefusesACharacterFacingNoWayAtAll)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {heroAt(antwika::gfx::Vec3{}, 0)};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["home"]["way"] = 8;

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_HoldsNoFloatEvenWithACamera)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .position = antwika::gfx::Vec3{1.5F, 2.5F, 3.5F},
                .yaw = 0.125F,
                .pitch = -0.875F},
        .zoom = 30};

    EXPECT_FALSE(holdsAFloat(nlohmann::json::parse(getSerializeMap(map))));
}

TEST(MapFileTest, ReadMap_TakesAMapThatKeptNoCamera)
{
    const auto loadedMap =
    getReadText(getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    EXPECT_FALSE(loadedMap.camera.has_value());
}

TEST(MapFileTest, ReadMap_TakesAMapWrittenBeforeCamerasWereKept)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("camera"));
    ageTo(document, 7);

    EXPECT_FALSE(getReadText(document.dump()).camera.has_value());
}

TEST(MapFileTest, ReadMap_RefusesACameraStandingAtAFraction)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["camera"] = {{"at", {0, 0, 0}}, {"yaw", 0.5},
                          {"pitch", 0}, {"zoom", 1000}};

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_GivesACameraKeptBeforeZoomTheOpeningOne)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform = antwika::camera::CameraTransform{}, .zoom = 30};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["camera"].erase(std::string("zoom"));
    ageTo(document, 8);

    const auto loadedMap = getReadText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom, antwika::camera::kDefaultZoom);
}

TEST(MapFileTest, ReadMap_RefusesACameraShowingNothing)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["camera"] = {{"at", {0, 0, 0}}, {"yaw", 0},
                          {"pitch", 0}, {"zoom", 0}};

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsHowTheEditorWasLeftStanding)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.settings = antwika::map::Settings{
        .lighting = false, .cornersJoined = true};

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(loadedMap.settings, map.settings);
}

TEST(MapFileTest, WriteMap_KeepsNoneOfTheWorkbenchTheArtistSetUp)
{
    Map map{.tilemap = getDefaultTilemap()};

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_TRUE(document["settings"]["lighting"].get<bool>());
    EXPECT_FALSE(document["settings"].contains("tool"));
    EXPECT_FALSE(document["settings"].contains("drawing"));
    EXPECT_FALSE(document["settings"].contains("view"));
    EXPECT_FALSE(document["settings"].contains("kind"));
}

TEST(MapFileTest, ReadMap_GivesAMapKeptBeforeSettingsTheOpeningOnes)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("settings"));
    ageTo(document, 9);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_EQ(loadedMap.settings, antwika::map::Settings{});
}

TEST(MapFileTest, ReadMap_RefusesAToolItDoesNotKnow)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["settings"]["tool"] = "chisel";

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsWhatEachVoxelIsMadeOf)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.voxels = antwika::voxel::voxelsOf({
        antwika::voxel::VoxelCell{.position = {.x = 0, .y = 0, .z = 0},
            .material = {.kind = antwika::voxel::Kind::Water}},
        antwika::voxel::VoxelCell{.position = {.x = 1, .y = 0, .z = 0},
            .material = {.kind = antwika::voxel::Kind::Ramp}}});

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 2U);
    EXPECT_EQ(
        loadedMap.voxels.at(antwika::voxel::VoxelPosition{}).kind,
        antwika::voxel::Kind::Water);
    EXPECT_EQ(
        loadedMap.voxels.at(antwika::voxel::VoxelPosition{.x = 1}).kind,
        antwika::voxel::Kind::Ramp);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["voxels"][0]["kind"], "water");
}

TEST(MapFileTest, ReadMap_MakesEveryVoxelKeptBeforeTheKindsASolidOne)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.voxels = voxelsOf({antwika::voxel::VoxelCell{.position = {.x = 3,
        .y = 2, .z = 1}}});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["voxels"] = nlohmann::json::array({{3, 2, 1}});
    document["settings"].erase(std::string("kind"));
    ageTo(document, 10);

    const auto loadedMap = getReadText(document.dump());

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(
        loadedMap.voxels
            .at(antwika::voxel::VoxelPosition{.x = 3, .y = 2, .z = 1})
            .kind,
        antwika::voxel::Kind::Normal);
}

TEST(MapFileTest, ReadMap_RefusesAKindItDoesNotKnow)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.voxels = voxelsOf({antwika::voxel::VoxelCell{}});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["voxels"][0]["kind"] = "lava";

    EXPECT_THROW((void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, WriteMap_KeepsWhichTilesAreGivenToWhichKind)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile poolTile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 4};

    map.rules.setKind(poolTile, antwika::voxel::Kind::Water);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(loadedMap.rules.kindOf(poolTile), antwika::voxel::Kind::Water);
    EXPECT_EQ(
        loadedMap.rules.kindOf(antwika::tilemap::Tile{}),
        antwika::voxel::Kind::Normal);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["tileKinds"][0]["kind"], "water");
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheKindsSolid)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("tileKinds"));
    ageTo(document, 11);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_TRUE(loadedMap.rules.getKinds().empty());
}

TEST(MapFileTest, SidecarPath_NamesTheAtlasAfterItsMap)
{
    EXPECT_EQ(
        antwika::map::getSidecarPath("map.json", "atlas-15x9.png"),
        "map-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_KeepsItInTheMapsOwnDirectory)
{
    EXPECT_EQ(
        antwika::map::getSidecarPath(
            "worlds/first.json", "atlas-15x12.png"),
        "worlds/first-atlas-15x12.png");
}

TEST(MapFileTest, SidecarPath_TakesAMapWithNoEndingWhole)
{
    EXPECT_EQ(
        antwika::map::getSidecarPath("worlds/first", "atlas-15x9.png"),
        "worlds/first-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_LeavesADottedDirectoryAlone)
{
    EXPECT_EQ(
        antwika::map::getSidecarPath("a.b/first", "atlas-15x9.png"),
        "a.b/first-atlas-15x9.png");
}

TEST(MapFileTest, SidecarPath_KeepsTwoMapsApart)
{
    EXPECT_NE(
        antwika::map::getSidecarPath("one.json", "atlas-15x9.png"),
        antwika::map::getSidecarPath("two.json", "atlas-15x9.png"));
}

TEST(MapFileTest, WriteMap_KeepsWhichWayATileWasDrawnFor)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 115};

    map.rules.setFacing(tile, antwika::voxel::Facing::West);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.facingOf(tile), antwika::voxel::Facing::West);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["tileFacings"][0]["facing"], "west");
}

TEST(MapFileTest, WriteMap_KeepsWhichLevelOfAFlightATileWasDrawnFor)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 160};

    map.rules.setLevel(tile, antwika::voxel::StairHalf::Lower);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.levelOf(tile), antwika::voxel::StairHalf::Lower);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["tileLevels"][0]["level"], "lower");
}

TEST(MapFileTest, WriteMap_KeepsWhichPartOfAFlightATileWasDrawnFor)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 160};

    map.rules.setPart(tile, antwika::voxel::StairPart::Side);

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        loadedMap.rules.partOf(tile), antwika::voxel::StairPart::Side);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["tileParts"][0]["part"], "side");
}

TEST(MapFileTest, WriteMap_KeepsHowStronglyADecorIsWeighed)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::getWithDecorToggled({}, tile);
    map.decor = antwika::decor::getWithWeightSet(map.decor, tile, 35);

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().weight, 35);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["decor"][0]["weight"], 35);
}

TEST(MapFileTest, WriteMap_KeepsWhichLayerADecorDressesFor)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::getWithDecorToggled({}, tile, 2);

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().layer, 2U);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["decor"][0]["layer"], 2);
}

TEST(MapFileTest, ReadMap_GathersDecorKeptBeforeTheLayersOntoTheFirst)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::getWithDecorToggled({}, tile, 2);

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["decor"][0].erase(std::string("layer"));
    ageTo(document, 39);

    const auto loadedMap = getReadText(document.dump());

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(loadedMap.decor.front().layer, 1U);
}

TEST(MapFileTest, ReadMap_GivesADecorKeptBeforeTheWeightsFullWeight)
{
    Map map{.tilemap = getDefaultTilemap()};
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Floor, .index = 26};

    map.decor = antwika::decor::getWithDecorToggled({}, tile);

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["decor"][0].erase(std::string("weight"));
    ageTo(document, 38);

    const auto loadedMap = getReadText(document.dump());

    ASSERT_EQ(loadedMap.decor.size(), 1U);
    EXPECT_EQ(
        loadedMap.decor.front().weight,
        antwika::decor::kFullFrequency);
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeThePartsToEitherPart)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("tileParts"));
    ageTo(document, 37);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_TRUE(loadedMap.rules.getParts().empty());
}

TEST(MapFileTest, ReadMap_TakesAZoomKeptAsHowMuchWorldStoodInView)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{.zoom = 45};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 14);
    document["camera"]["zoom"] = 9000;

    const auto loadedMap = getReadText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom, antwika::camera::kVoxelPixels);
}

TEST(MapFileTest, ReadMap_BringsAZoomBetweenScalesToAWholeOne)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{.zoom = 45};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 14);
    document["camera"]["zoom"] = 3600;

    const auto loadedMap = getReadText(document.dump());

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(loadedMap.camera->zoom % antwika::camera::kVoxelPixels, 0);
}

TEST(MapFileTest, WriteMap_KeepsWhichWayARampWasToldToClimb)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.voxels.merge(antwika::voxel::voxelsOf(
        {antwika::voxel::VoxelCell{.position = {.x = 1, .y = 0, .z = 2},
            .material = {.kind = antwika::voxel::Kind::Ramp,
                .facing = antwika::voxel::Facing::North}}}));

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(
        loadedMap.voxels
            .at(antwika::voxel::VoxelPosition{.x = 1, .y = 0, .z = 2})
            .facing,
        antwika::voxel::Facing::North);

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_EQ(document["voxels"][0]["climb"], "north");
}

TEST(MapFileTest, WriteMap_SaysNothingOfARampToldNothing)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.voxels.merge(antwika::voxel::voxelsOf(
        {antwika::voxel::VoxelCell{
            .material = {.kind = antwika::voxel::Kind::Ramp}}}));

    const auto document = nlohmann::json::parse(getSerializeMap(map));

    EXPECT_FALSE(document["voxels"][0].contains("climb"));

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.voxels.size(), 1U);
    EXPECT_EQ(
        loadedMap.voxels.at(antwika::voxel::VoxelPosition{}).facing,
        antwika::voxel::Facing::Any);
}

TEST(MapFileTest, ReadMap_MakesASquaredPitchExactAgain)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.camera = antwika::map::CameraView{
        .transform =
            antwika::camera::CameraTransform{
                .pitch = antwika::camera::getIsometricPitch()},
        .zoom = 45};

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_TRUE(loadedMap.camera.has_value());
    EXPECT_EQ(
        loadedMap.camera->transform.pitch, antwika::camera::getIsometricPitch());
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheLevelsToEither)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("tileLevels"));
    ageTo(document, 13);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_TRUE(loadedMap.rules.getLevels().empty());
}

TEST(MapFileTest, ReadMap_LeavesTilesKeptBeforeTheFacingsToAnyFlight)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("tileFacings"));
    ageTo(document, 12);

    const auto loadedMap = getReadText(document.dump());

    EXPECT_TRUE(loadedMap.rules.getFacings().empty());
}

TEST(MapFileTest, WriteMap_CarriesTheLampsSetDownAboutThePile)
{
    using antwika::light::Lamp;

    Map map{.tilemap = getDefaultTilemap()};

    map.lamps.push_back(
        Lamp{
            .position = VoxelPosition{.x = 2, .y = 3, .z = -4},
            .tintColor =
                antwika::gfx::Color{
                    .red = 12,
                    .green = 240,
                    .blue = 7,
                    .alpha = 200}});

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.lamps.size(), 1U);
    EXPECT_EQ(loadedMap.lamps.front(), map.lamps.front());
}

TEST(MapFileTest, WriteMap_WritesALampInWholeNumbers)
{
    using antwika::light::Lamp;

    Map map{.tilemap = getDefaultTilemap()};

    map.lamps.push_back(Lamp{.position = VoxelPosition{.x = -1, .y = 5}});

    EXPECT_FALSE(
        holdsAFloat(nlohmann::json::parse(getSerializeMap(map))));
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeLampsWithNone)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("lamps"));
    ageTo(document, 16);

    EXPECT_TRUE(getReadText(document.dump()).lamps.empty());
}

TEST(MapFileTest, ReadMap_RefusesMoreLampsThanTheWorldDrawsBy)
{
    using antwika::light::kMaxLamps;

    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

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
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeLayersHoldingTheBase)
{
    using antwika::map::kBaseLayer;
    using antwika::map::kBaseLayerName;

    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("layers"));
    ageTo(document, 17);

    const auto layers = getReadText(document.dump()).layers;

    ASSERT_EQ(layers.size(), 1U);
    EXPECT_EQ(layers.at(kBaseLayer).name, kBaseLayerName);
}

TEST(MapFileTest, WriteMap_CarriesTheLayersAMapIsDrawnIn)
{
    using antwika::map::Layer;

    Map map{.tilemap = getDefaultTilemap()};

    map.layers.push_back(Layer{.name = "another"});

    EXPECT_EQ(getReadText(getSerializeMap(map)).layers, map.layers);
}

TEST(MapFileTest, ReadMap_RefusesMoreLayersThanAMapMayHold)
{
    using antwika::map::kMaxLayers;

    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    auto layers = nlohmann::json::array();

    for (std::size_t index = 0; index <= kMaxLayers; ++index)
    {
        layers.push_back(nlohmann::json{{"name", "one"}});
    }

    document["layers"] = layers;

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            getReadText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesAPaletteAddedTo)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.paletteColors.push_back(
        antwika::gfx::Color{
            .red = 1, .green = 2, .blue = 3, .alpha = 4});

    EXPECT_EQ(getReadText(getSerializeMap(map)).paletteColors, map.paletteColors);
}

TEST(MapFileTest, WriteMap_CarriesAPaletteTakenFrom)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.paletteColors.resize(1);

    EXPECT_EQ(getReadText(getSerializeMap(map)).paletteColors, map.paletteColors);
}

TEST(MapFileTest, ReadMap_RefusesAPaletteOfNothingAtAll)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["palette"] = nlohmann::json::array();

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            getReadText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesMoreInksThanAMapMayHold)
{
    using antwika::tile::kMaxInks;

    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    auto colors = nlohmann::json::array();

    for (std::size_t index = 0; index <= kMaxInks; ++index)
    {
        colors.push_back(nlohmann::json::array({1, 2, 3, 4}));
    }

    document["palette"] = colors;

    EXPECT_THROW(
        [[maybe_unused]] const auto map =
            getReadText(document.dump()),
        MapFileError);
}

TEST(MapFileTest, ReadMap_LeavesAMapMixedBeforeWithTheInksItHad)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 18);

    EXPECT_EQ(
        getReadText(document.dump()).paletteColors.size(),
        antwika::tile::kPaletteSize);
}

TEST(MapFileTest, WriteMap_CarriesTheVariantFamilies)
{
    using antwika::decor::VariantGroup;
    using antwika::decor::VariantMember;

    auto map = getDemoMap();

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

    EXPECT_EQ(getReadText(getSerializeMap(map)).familyGroups, map.familyGroups);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeFamiliesWithNone)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("families"));
    ageTo(document, 30);

    EXPECT_TRUE(getReadText(document.dump()).familyGroups.empty());
}

TEST(MapFileTest, ReadMap_RefusesAWeightPastFullFrequency)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

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
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAGroupOfNoVariantsAtAll)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["families"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 1})},
            {"weight", 100},
            {"members", nlohmann::json::array()}}});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesASpannedDecorWhole)
{
    using antwika::tilemap::Atlas;

    auto map = getDemoMap();
    auto decor = antwika::decor::getWithDecorToggled(
        {}, Tile{.atlas = Atlas::Floor, .index = 3});

    decor = antwika::decor::getWithSpanSet(
        decor, Tile{.atlas = Atlas::Floor, .index = 3}, 2, 2);
    decor = antwika::decor::getWithMemberSet(
        decor,
        Tile{.atlas = Atlas::Floor, .index = 3},
        2,
        Tile{.atlas = Atlas::Floor, .index = 5});
    map.decor = decor;

    EXPECT_EQ(getReadText(getSerializeMap(map)).decor, map.decor);
}

TEST(MapFileTest, ReadMap_GivesADecorMarkedBeforeSpansOneFace)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.decor = antwika::decor::getWithDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    for (auto &entry : document["decor"])
    {
        entry.erase(std::string("span"));
        entry.erase(std::string("members"));
    }

    ageTo(document, 31);

    const auto reloadedMap = getReadText(document.dump());

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
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.decor = antwika::decor::getWithDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["decor"][0]["span"] =
        nlohmann::json::array({5, 1});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesSpanTilesThatDoNotFillTheSpan)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.decor = antwika::decor::getWithDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["decor"][0]["span"] =
        nlohmann::json::array({2, 2});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheFlipsOfItsTiles)
{
    using antwika::tilemap::Atlas;

    auto map = getDemoMap();
    auto flips = antwika::decor::getWithAnimationToggled(
        {}, Tile{.atlas = Atlas::Floor, .index = 3});

    flips = antwika::decor::getWithAnimationFrameAdded(
        flips, Tile{.atlas = Atlas::Floor, .index = 3});
    flips = antwika::decor::getWithAnimationFrameSet(
        flips,
        Tile{.atlas = Atlas::Floor, .index = 3},
        1,
        Tile{.atlas = Atlas::Floor, .index = 9});
    map.flipAnimations = flips;

    EXPECT_EQ(getReadText(getSerializeMap(map)).flipAnimations, map.flipAnimations);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeFlipsStill)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("flips"));
    ageTo(document, 32);

    EXPECT_TRUE(getReadText(document.dump()).flipAnimations.empty());
}

TEST(MapFileTest, ReadMap_RefusesAFlipOfNineFrames)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

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
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAFlipAcrossTheAtlases)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["flips"] = nlohmann::json::array(
        {nlohmann::json{
            {"tile", nlohmann::json::array({"flat", 0})},
            {"frames",
             nlohmann::json::array(
                 {nlohmann::json::array({"flat", 0}),
                  nlohmann::json::array({"upright", 1})})}}});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheTransitionsItWove)
{
    using antwika::tilemap::Atlas;

    auto map = getDemoMap();

    map.transitions = {
        antwika::tile::TransitionTile{
            .fromTile = Tile{.atlas = Atlas::Floor, .index = 1},
            .toTile = Tile{.atlas = Atlas::Floor, .index = 2},
            .maskTile = Tile{.atlas = Atlas::Floor, .index = 8},
            .outputTile = Tile{.atlas = Atlas::Floor, .index = 9}}};

    EXPECT_EQ(
        getReadText(getSerializeMap(map)).transitions, map.transitions);
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeTransitionsBare)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("transitions"));
    ageTo(document, 33);

    EXPECT_TRUE(getReadText(document.dump()).transitions.empty());
}

TEST(MapFileTest, ReadMap_RefusesATransitionAcrossAtlases)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["transitions"] = nlohmann::json::array(
        {nlohmann::json{
            {"from", nlohmann::json::array({"upright", 1})},
            {"to", nlohmann::json::array({"flat", 2})},
            {"mask", nlohmann::json::array({"flat", 8})},
            {"slot", nlohmann::json::array({"flat", 9})}}});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesATransitionIntoItself)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["transitions"] = nlohmann::json::array(
        {nlohmann::json{
            {"from", nlohmann::json::array({"flat", 1})},
            {"to", nlohmann::json::array({"flat", 1})},
            {"mask", nlohmann::json::array({"flat", 8})},
            {"slot", nlohmann::json::array({"flat", 9})}}});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesTheMarkersItHolds)
{
    auto map = getDemoMap();

    std::int32_t apart = 1;

    for (const auto marker : antwika::map::kEveryMarker)
    {
        map.markers.positionsOf(marker) = {
            VoxelPosition{.x = apart, .y = 0, .z = 2},
            VoxelPosition{.x = apart, .y = 1, .z = 2}};
        apart += 2;
    }

    const auto reloadedMap = getReadText(getSerializeMap(map));

    for (const auto marker : antwika::map::kEveryMarker)
    {
        EXPECT_EQ(
            reloadedMap.markers.positionsOf(marker), map.markers.positionsOf(marker))
            << static_cast<int>(marker);
    }
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeMarkersBare)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document.erase(std::string("checkpoints"));
    ageTo(document, 34);

    const auto reloadedMap = getReadText(document.dump());

    EXPECT_TRUE(reloadedMap.markers.positionsOf(antwika::map::Marker::Checkpoint).empty());
}

TEST(MapFileTest, ReadMap_DropsTheGatesAMapHeldBeforeTheirRemoval)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 48);
    document["keys"] = nlohmann::json::array(
        {nlohmann::json::array({1, 0, 2})});
    document["doors"] = nlohmann::json::array(
        {nlohmann::json::array({3, 0, 2})});
    document["plates"] = nlohmann::json::array(
        {nlohmann::json::object(
            {{"at", nlohmann::json::array({5, 0, 2})},
             {"sways", nlohmann::json::array()}})});
    document["exitLocked"] = true;

    const auto reloadedMap = getReadText(document.dump());

    for (const auto marker : antwika::map::kEveryMarker)
    {
        EXPECT_TRUE(reloadedMap.markers.positionsOf(marker).empty());
    }
}

TEST(MapFileTest, WriteMap_CarriesTheItemsItHolds)
{
    auto map = getDemoMap();

    map.markers.positionsOf(antwika::map::Marker::Food) = {VoxelPosition{.x = 1, .y = 0, .z = 2}};
    map.markers.positionsOf(antwika::map::Marker::Water) = {
        VoxelPosition{.x = 3, .y = 0, .z = 2},
        VoxelPosition{.x = 3, .y = 1, .z = 2}};

    const auto reloadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        reloadedMap.markers.positionsOf(antwika::map::Marker::Food),
        map.markers.positionsOf(antwika::map::Marker::Food));
    EXPECT_EQ(
        reloadedMap.markers.positionsOf(antwika::map::Marker::Water),
        map.markers.positionsOf(antwika::map::Marker::Water));
}

TEST(MapFileTest, ReadMap_LeavesAMapDrawnBeforeItemsBare)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 41);

    const auto reloadedMap = getReadText(document.dump());

    EXPECT_TRUE(reloadedMap.markers.positionsOf(antwika::map::Marker::Food).empty());
    EXPECT_TRUE(reloadedMap.markers.positionsOf(antwika::map::Marker::Water).empty());
}

TEST(MapFileTest, ReadMap_RefusesAnItemBeyondTheLattice)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["water"] = nlohmann::json::array(
        {nlohmann::json::array({kMaxCellCoord + 1, 0, 0})});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, WriteMap_CarriesWhetherTheCornersAreJoined)
{
    auto map = getDemoMap();

    map.settings.cornersJoined = true;

    EXPECT_TRUE(getReadText(getSerializeMap(map)).settings.cornersJoined);
}

TEST(MapFileTest, ReadMap_RefusesACheckpointBeyondTheLattice)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    document["checkpoints"] = nlohmann::json::array(
        {nlohmann::json::array(
            {kMaxCellCoord + 1, 0, 0})});

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

[[nodiscard]] bool carries(
    const antwika::map::Character &character,
    const std::string_view name)
{
    return std::ranges::find(character.components, name)
           != character.components.end();
}

TEST(MapFileTest, WriteMap_CarriesTheComponentsACharacterNames)
{
    auto map = getDemoMap();

    map.characters = {
        antwika::map::Character{.components = {"component::CarriedLight"}}};

    EXPECT_TRUE(carries(
        getReadText(getSerializeMap(map)).characters.at(0),
        "component::CarriedLight"));
}

TEST(MapFileTest, ReadMap_LeavesAFigureDrawnBeforeLampsCarryingNothing)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 35);

    for (auto &figure : document["figures"])
    {
        figure.erase(std::string("lamp"));
    }

    EXPECT_FALSE(carries(
        getReadText(document.dump()).characters.at(0),
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

        const auto loadedMap = getReadText(text);
        const auto hero = antwika::map::getPlayerIndex(loadedMap);

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
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 42);

    for (auto &figure : document["characters"])
    {
        figure["lamp"] = true;
    }

    EXPECT_TRUE(carries(
        getReadText(document.dump()).characters.at(0),
        "component::CarriedLight"));
}

TEST(MapFileTest, ReadMap_MovesOlderComponentNamesIntoComponent)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 44);

    for (auto &figure : document["characters"])
    {
        figure["components"] = nlohmann::json::array(
            {"collision::Position",
             "collision::Velocity",
             "collision::Player",
             "character::AnimationState",
             "character::CharacterIndex",
             "character::Speaker",
             "light::CarriedLight",
             "light::FillLight"});
    }

    const auto loadedCharacter =
        getReadText(document.dump()).characters.at(0);

    EXPECT_TRUE(carries(loadedCharacter, "component::Position"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Velocity"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Player"));
    EXPECT_TRUE(carries(loadedCharacter, "component::AnimationState"));
    EXPECT_TRUE(carries(loadedCharacter, "component::CharacterIndex"));
    EXPECT_TRUE(carries(loadedCharacter, "component::Speaker"));
    EXPECT_TRUE(carries(loadedCharacter, "component::CarriedLight"));
    EXPECT_TRUE(carries(loadedCharacter, "component::FillLight"));
}

TEST(MapFileTest, ReadMap_LeavesAComponentItDoesNotRenameAlone)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 44);

    for (auto &figure : document["characters"])
    {
        figure["components"] =
            nlohmann::json::array({"component::Health"});
    }

    EXPECT_TRUE(carries(
        getReadText(document.dump()).characters.at(0),
        "component::Health"));
}

TEST(MapFileTest, ReadMap_RefusesFigureComponentsOfNumbers)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["components"] = {3};

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_LetsGoOfWhatADecorWasCalled)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.decor = antwika::decor::getWithDecorToggled(
        {}, Tile{.atlas = antwika::tilemap::Atlas::Floor, .index = 3});

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["decor"][0]["name"] = "moss";
    ageTo(document, 36);

    EXPECT_EQ(getReadText(document.dump()).decor, map.decor);
}

TEST(MapFileTest, SaveMap_KeepsWhatItWroteOverUnderTheBackupName)
{
    const auto path = getSomewhereToWrite("antwika-backup-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);
    auto first = getDemoMap();

    first.ambient = 11;

    auto second = first;

    second.ambient = 77;

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    saveMap(path, first);

    EXPECT_FALSE(std::filesystem::exists(backupPath));

    saveMap(path, second);

    ASSERT_TRUE(std::filesystem::exists(backupPath));
    EXPECT_EQ(getLoadMap(path), second);
    EXPECT_EQ(getLoadMap(backupPath), first);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SaveMap_LeavesNothingLyingHalfWritten)
{
    const auto path = getSomewhereToWrite("antwika-halfway-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);
    const auto writingPath =
        path + std::string(antwika::map::kWritingSuffix);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    saveMap(path, getDemoMap());

    EXPECT_FALSE(std::filesystem::exists(writingPath));

    saveMap(path, getDemoMap());

    EXPECT_FALSE(std::filesystem::exists(writingPath));

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SaveMap_KeepsOnlyTheLastMapItWroteOver)
{
    const auto path = getSomewhereToWrite("antwika-onebak-map.json");
    const auto backupPath =
        path + std::string(antwika::map::kBackupSuffix);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);

    for (const std::int32_t ambient : {1, 2, 3})
    {
        auto map = getDemoMap();

        map.ambient = ambient;
        saveMap(path, map);
    }

    EXPECT_EQ(getLoadMap(path).ambient, 3);
    EXPECT_EQ(getLoadMap(backupPath).ambient, 2);

    std::filesystem::remove(path);
    std::filesystem::remove(backupPath);
}

TEST(MapFileTest, SharedTexturePath_PutsItBesideTheMapsOwnFolder)
{
    EXPECT_EQ(
        antwika::map::getSharedTexturePath(
            "assets/maps/map.json", "character-20x28.png"),
        "assets/textures/character-20x28.png");
}

TEST(MapFileTest, SharedTexturePath_IsTheSameForEveryMapOfAFolder)
{
    EXPECT_EQ(
        antwika::map::getSharedTexturePath(
            "assets/maps/one.json", "icons-16.png"),
        antwika::map::getSharedTexturePath(
            "assets/maps/another.json", "icons-16.png"));
}

TEST(MapFileTest, SharedTexturePath_KeepsTheNameWhole)
{
    EXPECT_EQ(
        std::filesystem::path(
            antwika::map::getSharedTexturePath(
                "assets/maps/map.json", "icons-16.png"))
            .filename()
            .string(),
        "icons-16.png");
}

TEST(MapFileTest, SharedTexturePath_IsNotBesideTheMapItself)
{
    const std::string mapPath = "assets/maps/map.json";

    EXPECT_NE(
        antwika::map::getSharedTexturePath(
            mapPath, "character-20x28.png"),
        antwika::map::getSidecarPath(mapPath, "character-20x28.png"));
}

TEST(MapFileTest, WriteMap_KeepsTheRosterOfCharacters)
{
    auto map = getDemoMap();

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel"}};

    const auto loadedMap = getReadText(getSerializeMap(map));

    ASSERT_EQ(loadedMap.characters.size(), 2U);
    EXPECT_EQ(loadedMap.characters.at(0).name, "Ada");
    EXPECT_EQ(loadedMap.characters.at(1).name, "Bel");
}

TEST(MapFileTest, WriteMap_MarksThePlayerAmongTheCharacters)
{
    auto map = getDemoMap();

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel", .player = true}};

    const auto loadedMap = getReadText(getSerializeMap(map));

    EXPECT_EQ(
        antwika::map::getPlayerIndex(loadedMap),
        std::optional<std::size_t>{1});
}

TEST(MapFileTest, WriteMap_NoLongerWritesAWalkerBesideTheRoster)
{
    const auto document = nlohmann::json::parse(
        getSerializeMap(getDemoMap()));

    EXPECT_FALSE(document.contains("walker"));
}

TEST(MapFileTest, ReadMap_TurnsAwayAMapWithTwoPlayers)
{
    auto map = getDemoMap();

    map.characters = {
        antwika::map::Character{.player = true},
        antwika::map::Character{.player = true}};

    EXPECT_THROW(
        (void)getReadText(getSerializeMap(map)),
        antwika::map::MapFileError);
}

TEST(MapFileTest, ReadMap_MakesAPlayerOfTheWalkerOfAnOlderMap)
{
    auto document = nlohmann::json::parse(
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 40);
    document["walker"] = {{"at", {1000, 2000, 3000}}, {"way", 3}};

    const auto loadedMap = getReadText(document.dump());
    const auto hero = antwika::map::getPlayerIndex(loadedMap);

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
        getSerializeMap(Map{.tilemap = getDefaultTilemap()}));

    ageTo(document, 40);

    EXPECT_FALSE(
        antwika::map::getPlayerIndex(getReadText(document.dump()))
            .has_value());
}

TEST(MapFileTest, ReadMap_KeepsTheFiguresOfAnOlderMapBeforeThePlayer)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {
        antwika::map::Character{.name = "Ada"},
        antwika::map::Character{.name = "Bel"}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 40);
    document["walker"] = {{"at", {0, 0, 0}}, {"way", 0}};

    const auto loadedMap = getReadText(document.dump());

    ASSERT_EQ(loadedMap.characters.size(), 3U);
    EXPECT_EQ(loadedMap.characters.at(0).name, "Ada");
    EXPECT_EQ(loadedMap.characters.at(1).name, "Bel");
    EXPECT_EQ(
        antwika::map::getPlayerIndex(loadedMap),
        std::optional<std::size_t>{2});
}

TEST(MapFileTest, ReadMap_TurnsAFigureOfAnOlderMapTheWayItStood)
{
    const std::vector<std::pair<int, std::uint8_t>> turns{
        {0, 2}, {1, 4}, {2, 0}, {3, 6}};

    for (const auto &[was, becomes] : turns)
    {
        auto map = Map{.tilemap = getDefaultTilemap()};

        map.characters = {antwika::map::Character{}};

        auto document = nlohmann::json::parse(getSerializeMap(map));

        ageTo(document, 40);
        document["figures"][0]["home"]["way"] = was;

        EXPECT_EQ(
            getReadText(document.dump())
                .characters.at(0)
                .idlePlacement.way,
            becomes);
    }
}

TEST(MapFileTest, ReadMap_RefusesACharacterPlayerFlagOfNumbers)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["player"] = 3;

    EXPECT_THROW(
        (void)getReadText(document.dump()),
        antwika::map::MapFileError);
}

TEST(MapFileTest, PlayerIndex_FindsTheOnePlayerOfARoster)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {
        antwika::map::Character{},
        antwika::map::Character{.player = true}};

    EXPECT_EQ(
        antwika::map::getPlayerIndex(map),
        std::optional<std::size_t>{1});
}

TEST(MapFileTest, PlayerIndex_GivesNothingWhereNoneIsMarked)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    EXPECT_FALSE(antwika::map::getPlayerIndex(map).has_value());
}

TEST(MapFileTest, PatrolStopsOf_GivesEveryCharacterItsStopsInOrder)
{
    Map map{.tilemap = getDefaultTilemap()};

    map.characters = {
        antwika::map::Character{
            .patrolPathPositions = {VoxelPosition{.x = 1, .y = 2, .z = 3}}},
        antwika::map::Character{}};

    const auto stops = antwika::map::patrolStopsOf(map);

    ASSERT_EQ(stops.size(), 2U);
    ASSERT_EQ(stops.at(0).size(), 1U);
    EXPECT_EQ(stops.at(0).at(0).x, 1);
    EXPECT_TRUE(stops.at(1).empty());
}

TEST(MapFileTest, WriteMap_ReadsBackEveryWorldSettingItWrote)
{
    for (const auto lighting : {false, true})
    {
        for (const auto joined : {false, true})
        {
            Map map;
            map.settings.lighting = lighting;
            map.settings.cornersJoined = joined;

            std::ostringstream writtenText;
            writeMap(writtenText, map);

            std::istringstream stream(writtenText.str());
            const auto loadedMap = readMap(stream);

            EXPECT_EQ(loadedMap.settings.lighting, lighting);
            EXPECT_EQ(loadedMap.settings.cornersJoined, joined);
        }
    }
}

TEST(MapFileTest, WriteMap_CarriesTheValuesACharacterSets)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    antwika::map::Character character;

    character.components = {
        "component::Health", "component::CarriedLight"};
    character.componentValues.insert_or_assign(
        "component::Health",
        antwika::loadout::ComponentValue(
            antwika::component::Health{.food = 30, .water = 200}));

    auto light = antwika::component::CarriedLight{};

    light.tintColor = antwika::gfx::Color{
        .red = 9, .green = 8, .blue = 7, .alpha = 255};
    character.componentValues.insert_or_assign(
        "component::CarriedLight",
        antwika::loadout::ComponentValue(light));
    map.characters = {character};

    const auto text = getSerializeMap(map);
    const auto document = nlohmann::json::parse(text);
    const auto &values =
        document["characters"][0]["componentValues"];

    EXPECT_EQ(values["component::Health"]["food"], 30);
    EXPECT_EQ(values["component::Health"]["water"], 200);
    EXPECT_EQ(
        values["component::CarriedLight"]["tint"],
        nlohmann::json::array({9, 8, 7, 255}));
    EXPECT_TRUE(values["component::CarriedLight"].contains("above"));
    EXPECT_TRUE(values["component::CarriedLight"].contains("reach"));

    EXPECT_EQ(getReadText(text).characters, map.characters);
}

TEST(MapFileTest, ReadMap_GivesACharacterKeptBeforeValuesNoneAtAll)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {
        antwika::map::Character{},
        antwika::map::Character{.name = "Watcher"}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    ageTo(document, 47);

    for (const auto &figure : document["characters"])
    {
        EXPECT_FALSE(figure.contains("tuning"));
    }

    const auto loadedMap = getReadText(document.dump());

    ASSERT_EQ(loadedMap.characters.size(), 2U);
    EXPECT_TRUE(loadedMap.characters.at(0).componentValues.empty());
    EXPECT_TRUE(loadedMap.characters.at(1).componentValues.empty());
}

TEST(MapFileTest, ReadMap_RefusesAValueOfAComponentItDoesNotKnow)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["componentValues"]
            ["component::Missing"] =
        nlohmann::json::object();

    EXPECT_THROW(
        (void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAValueFieldItDoesNotKnow)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["componentValues"]
            ["component::Health"] = {
        {"food", 1}, {"water", 1}, {"mana", 1}};

    EXPECT_THROW(
        (void)getReadText(document.dump()), MapFileError);
}

TEST(MapFileTest, ReadMap_RefusesAValueOfATagComponent)
{
    auto map = Map{.tilemap = getDefaultTilemap()};

    map.characters = {antwika::map::Character{}};

    auto document = nlohmann::json::parse(getSerializeMap(map));

    document["characters"][0]["componentValues"]
            ["component::Player"] =
        nlohmann::json::object();

    EXPECT_THROW(
        (void)getReadText(document.dump()), MapFileError);
}
