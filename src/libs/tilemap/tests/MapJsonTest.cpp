#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <variant>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/FlowDirection.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/MapJson.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>
#include <antwika/tilemap/TileMapError.hpp>

using antwika::geometry::GridCell;
using antwika::tilemap::BoatEmbark;
using antwika::tilemap::FlowDirection;
using antwika::tilemap::MapDocument;
using antwika::tilemap::mapDocumentFromJson;
using antwika::tilemap::MapHeader;
using antwika::tilemap::Npc;
using antwika::tilemap::Overlay;
using antwika::tilemap::Pickup;
using antwika::tilemap::Slab;
using antwika::tilemap::SpawnPoint;
using antwika::tilemap::TerrainClass;
using antwika::tilemap::TileMap;
using antwika::tilemap::tileMapFromJson;
using antwika::tilemap::TileMapError;
using antwika::tilemap::toJson;
using antwika::tilemap::Transition;
using antwika::tilemap::TriggerVolume;

namespace
{
    [[nodiscard]] MapDocument documentOf()
    {
        MapDocument document{
            .map = TileMap(MapHeader{.id = "mask"}, 3, 2)};

        document.map.at(GridCell{.column = 1, .row = 0})
            .top()
            ->terrain = TerrainClass::Water;
        document.free = {false, true, false, true, false, true};

        return document;
    }

    [[nodiscard]] nlohmann::json legacyDocumentOf(
        const std::int64_t schema)
    {
        nlohmann::json water;
        water["at"] = nlohmann::json::array({1, 0});
        water["deadly"] = true;
        water["swimmable"] = true;
        water["current"] = "east";

        nlohmann::json out;
        out["schema"] = schema;
        out["id"] = "legacy";
        out["ink"] = nlohmann::json::array({0, 0, 0});
        out["paper"] = nlohmann::json::array({255, 255, 255});
        out["columns"] = 3;
        out["rows"] = 2;
        out["terrain"] = nlohmann::json::array({"f~f", "fff"});
        out["height"] = {{0, 0, 2}, {0, 0, 0}};
        out["light"] = {{255, 255, 255}, {255, 200, 255}};
        out["bridges"] =
            nlohmann::json::array({nlohmann::json::array({2, 0})});
        out["water"] = nlohmann::json::array({water});
        out["entities"] = nlohmann::json::array();

        if (schema >= 2)
        {
            out["free"] = nlohmann::json::array({"...", "..."});
        }

        if (schema >= 3)
        {
            out["tilesets"] = nlohmann::json::object();
        }

        return out;
    }

    [[nodiscard]] TileMap layeredMap()
    {
        TileMap map(MapHeader{.id = "layers"}, 2, 2);

        (void)map.at(GridCell{.column = 0, .row = 0})
            .place(Slab{
                .level = -2,
                .terrain = TerrainClass::Water,
                .water = {
                    .deadly = true,
                    .swimmable = true,
                    .current = FlowDirection::East}});
        (void)map.at(GridCell{.column = 0, .row = 0})
            .place(Slab{
                .level = 1,
                .terrain = TerrainClass::Path,
                .overlay = Overlay::Bridge,
                .light = 9});
        (void)map.at(GridCell{.column = 1, .row = 0})
            .place(Slab{
                .level = 0,
                .terrain = TerrainClass::Water,
                .water = {.swimmable = true}});
        map.at(GridCell{.column = 1, .row = 1}).clear();
        map.addEntity(Transition{
            .id = "door",
            .at = GridCell{.column = 0, .row = 1},
            .level = 1,
            .targetMap = "wakewater-02",
            .targetEntry = "west",
            .requiredTags = {"boss_key"}});
        map.addEntity(BoatEmbark{
            .id = "pier",
            .at = GridCell{.column = 1, .row = 0},
            .level = 0});
        map.addEntity(SpawnPoint{
            .id = "nest",
            .at = GridCell{.column = 0, .row = 0},
            .level = -2,
            .enemy = "crab"});
        map.addEntity(Pickup{
            .id = "coin",
            .at = GridCell{.column = 1, .row = 1},
            .level = 0,
            .item = "coin",
            .grantedTags = {"rich"}});
        map.addEntity(Npc{
            .id = "keeper",
            .at = GridCell{.column = 1, .row = 0},
            .level = 1});
        map.addEntity(TriggerVolume{
            .id = "zone",
            .at = GridCell{.column = 0, .row = 0},
            .level = 1,
            .columns = 2,
            .rows = 2,
            .event = "enter",
            .grantedTags = {}});

        return map;
    }
}

TEST(MapJsonTest, ToJson_AlwaysWritesSchemaFourWithAFreeSection)
{
    TileMap map(
        MapHeader{.id = "plain", .schemaVersion = 1}, 2, 2);

    const auto out = toJson(map);

    EXPECT_EQ(out.at("schema").get<std::uint32_t>(), 4U);
    ASSERT_EQ(out.at("free").size(), 2U);
    EXPECT_EQ(out.at("free").at(0).get<std::string>(), "..");
    EXPECT_EQ(out.at("free").at(1).get<std::string>(), "..");
}

TEST(MapJsonTest, ToJson_AlwaysWritesSchemaFourWithATilesetsSection)
{
    TileMap map(
        MapHeader{.id = "plain", .schemaVersion = 1}, 2, 2);

    const auto out = toJson(map);

    EXPECT_EQ(out.at("schema").get<std::uint32_t>(), 4U);
    ASSERT_TRUE(out.at("tilesets").is_object());
    EXPECT_TRUE(out.at("tilesets").empty());
}

TEST(MapJsonTest, ToJson_WritesOneLevelsEntryPerOccupiedLevel)
{
    TileMap map(MapHeader{.id = "levels"}, 2, 1);

    (void)map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 2});
    (void)map.at(GridCell{.column = 1, .row = 0})
        .place(Slab{.level = -1});

    const auto out = toJson(map);
    const auto &levels = out.at("levels");

    ASSERT_EQ(levels.size(), 3U);
    EXPECT_EQ(levels.at(0).at("level").get<std::int32_t>(), -1);
    EXPECT_EQ(levels.at(1).at("level").get<std::int32_t>(), 0);
    EXPECT_EQ(levels.at(2).at("level").get<std::int32_t>(), 2);
}

TEST(MapJsonTest, ToJson_WritesDotWhereAColumnHoldsNoSlab)
{
    TileMap map(MapHeader{.id = "dots"}, 2, 1);

    (void)map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 2});
    (void)map.at(GridCell{.column = 1, .row = 0})
        .place(Slab{.level = -1});

    const auto out = toJson(map);
    const auto &levels = out.at("levels");

    ASSERT_EQ(levels.size(), 3U);
    EXPECT_EQ(
        levels.at(0).at("terrain"),
        nlohmann::json::array({".f"}));
    EXPECT_EQ(
        levels.at(1).at("terrain"),
        nlohmann::json::array({"ff"}));
    EXPECT_EQ(
        levels.at(2).at("terrain"),
        nlohmann::json::array({"f."}));
}

TEST(MapJsonTest, ToJson_OmitsEmptyLightBridgesAndWaterMembers)
{
    TileMap map(MapHeader{.id = "bare"}, 1, 1);

    const auto out = toJson(map);
    const auto &entry = out.at("levels").at(0);

    EXPECT_TRUE(entry.contains("terrain"));
    EXPECT_FALSE(entry.contains("light"));
    EXPECT_FALSE(entry.contains("bridges"));
    EXPECT_FALSE(entry.contains("water"));
}

TEST(MapJsonTest, ToJson_WritesLightOnlyBelowFull)
{
    TileMap map(MapHeader{.id = "lit"}, 2, 1);

    map.at(GridCell{.column = 0, .row = 0}).top()->light = 200;

    const auto out = toJson(map);

    EXPECT_EQ(
        out.at("levels").at(0).at("light"),
        nlohmann::json::array({nlohmann::json::array({0, 0, 200})}));
}

TEST(MapJsonTest, ToJson_WritesEveryEntityWithItsLevel)
{
    TileMap map(MapHeader{.id = "peopled"}, 3, 2);

    map.addEntity(Transition{.id = "door", .level = 1});
    map.addEntity(BoatEmbark{.id = "pier", .level = -1});
    map.addEntity(SpawnPoint{.id = "nest", .level = 2});
    map.addEntity(Pickup{.id = "coin", .level = 0});
    map.addEntity(Npc{.id = "keeper", .level = 5});
    map.addEntity(TriggerVolume{.id = "zone", .level = -3});

    const auto out = toJson(map);
    const auto &entities = out.at("entities");

    ASSERT_EQ(entities.size(), 6U);
    EXPECT_EQ(entities.at(0).at("level").get<std::int32_t>(), 1);
    EXPECT_EQ(entities.at(1).at("level").get<std::int32_t>(), -1);
    EXPECT_EQ(entities.at(2).at("level").get<std::int32_t>(), 2);
    EXPECT_EQ(entities.at(3).at("level").get<std::int32_t>(), 0);
    EXPECT_EQ(entities.at(4).at("level").get<std::int32_t>(), 5);
    EXPECT_EQ(entities.at(5).at("level").get<std::int32_t>(), -3);
}

TEST(MapJsonTest, ToJson_IsByteStableAcrossARoundTrip)
{
    const auto out = toJson(layeredMap());

    const auto loaded = tileMapFromJson(out);

    EXPECT_EQ(toJson(loaded).dump(), out.dump());
}

TEST(MapJsonTest, TileMapFromJson_RoundTripsAMultiLevelMap)
{
    const auto map = layeredMap();

    const auto loaded = tileMapFromJson(toJson(map));

    for (std::uint32_t row = 0; row < map.rows(); ++row)
    {
        for (std::uint32_t column = 0; column < map.columns();
             ++column)
        {
            const auto cell =
                GridCell{.column = column, .row = row};

            EXPECT_EQ(loaded.at(cell), map.at(cell));
        }
    }

    EXPECT_EQ(loaded.entities(), map.entities());
}

TEST(MapJsonTest, TileMapFromJson_LoadsAnEmptyLevelsArrayAsEmptyColumns)
{
    TileMap map(MapHeader{.id = "void"}, 2, 1);

    map.at(GridCell{.column = 0, .row = 0}).clear();
    map.at(GridCell{.column = 1, .row = 0}).clear();

    const auto out = toJson(map);

    EXPECT_EQ(out.at("levels"), nlohmann::json::array());

    const auto loaded = tileMapFromJson(out);

    EXPECT_TRUE(
        loaded.at(GridCell{.column = 0, .row = 0}).slabs().empty());
    EXPECT_TRUE(
        loaded.at(GridCell{.column = 1, .row = 0}).slabs().empty());
}

TEST(MapJsonTest, TileMapFromJson_ReadsAnExplicitEntityLevel)
{
    TileMap map(MapHeader{.id = "entity"}, 1, 1);

    map.addEntity(Npc{.id = "keeper"});

    auto out = toJson(map);
    out["entities"].at(0)["level"] = 7;

    const auto loaded = tileMapFromJson(out);

    EXPECT_EQ(std::get<Npc>(loaded.entities().front()).level, 7);
}

TEST(MapJsonTest, TileMapFromJson_LoadsAVersionFourDocument)
{
    const auto out = toJson(documentOf());

    const auto map = tileMapFromJson(out);

    EXPECT_EQ(map.columns(), 3U);
    EXPECT_EQ(map.rows(), 2U);
    EXPECT_EQ(
        map.at(GridCell{.column = 1, .row = 0}).top()->terrain,
        TerrainClass::Water);
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
        loaded.map.at(GridCell{.column = 1, .row = 0})
            .top()
            ->terrain,
        TerrainClass::Water);
}

TEST(MapJsonTest, MapDocumentFromJson_LoadsAVersionOneMapAllPinned)
{
    const auto out = legacyDocumentOf(1);

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

    out["schema"] = 5;

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RoundTripsTheTilesetBindings)
{
    const TileMap bound(
        MapHeader{
            .id = "mask",
            .tilesets = {"meadow", "", "sea", "", "", ""}},
        3, 2);

    const auto out = toJson(bound);

    EXPECT_EQ(
        out.at("tilesets"),
        (nlohmann::json{{"floor", "meadow"}, {"water", "sea"}}));

    const auto loaded = tileMapFromJson(out);

    EXPECT_EQ(loaded.header().tilesets, bound.header().tilesets);
}

TEST(MapJsonTest, TileMapFromJson_LoadsAVersionTwoMapWithEveryTerrainUnbound)
{
    const auto out = legacyDocumentOf(2);

    const auto loaded = tileMapFromJson(out);

    for (const auto &tileset : loaded.header().tilesets)
    {
        EXPECT_EQ(tileset, "");
    }
}

TEST(MapJsonTest, TileMapFromJson_RefusesAVersionThreeMapLackingTilesets)
{
    auto out = legacyDocumentOf(3);

    out.erase("tilesets");

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RefusesATilesetsSectionThatIsNotAnObject)
{
    auto out = toJson(documentOf());

    out["tilesets"] = nlohmann::json::array();

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RefusesATilesetsEntryNamingAnUnknownTerrain)
{
    auto out = toJson(documentOf());

    out["tilesets"]["lava"] = "ember";

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RefusesATilesetsEntryWithANonStringValue)
{
    auto out = toJson(documentOf());

    out["tilesets"]["floor"] = 7;

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_SolidFillsAVersionThreeMapUpToEachHeight)
{
    const auto map = tileMapFromJson(legacyDocumentOf(3));

    const auto &tall = map.at(GridCell{.column = 2, .row = 0});
    ASSERT_EQ(tall.slabs().size(), 3U);
    EXPECT_EQ(tall.slabAt(0)->terrain, TerrainClass::Floor);
    EXPECT_EQ(tall.slabAt(1)->terrain, TerrainClass::Floor);
    EXPECT_EQ(tall.top()->level, 2);

    EXPECT_EQ(
        map.at(GridCell{.column = 0, .row = 0}).slabs().size(), 1U);
    EXPECT_EQ(
        map.at(GridCell{.column = 1, .row = 0}).top()->terrain,
        TerrainClass::Water);
    EXPECT_EQ(
        map.at(GridCell{.column = 1, .row = 1}).top()->light, 200);
}

TEST(MapJsonTest, TileMapFromJson_SolidFillsFromTheLowestNegativeHeight)
{
    auto out = legacyDocumentOf(3);

    out["height"] = {{-2, 0, 1}, {0, 0, 0}};

    const auto map = tileMapFromJson(out);

    const auto &sunken = map.at(GridCell{.column = 0, .row = 0});
    ASSERT_EQ(sunken.slabs().size(), 1U);
    EXPECT_EQ(sunken.top()->level, -2);

    const auto &middle = map.at(GridCell{.column = 1, .row = 0});
    ASSERT_EQ(middle.slabs().size(), 3U);
    EXPECT_NE(middle.slabAt(-2), nullptr);
    EXPECT_EQ(middle.top()->level, 0);

    EXPECT_EQ(
        map.at(GridCell{.column = 2, .row = 0}).slabs().size(), 4U);
}

TEST(MapJsonTest,
     TileMapFromJson_MigratesOverlayWaterAndLightOntoTheTopSlabOnly)
{
    auto out = legacyDocumentOf(3);

    out["light"] = {{255, 255, 100}, {255, 255, 255}};

    const auto map = tileMapFromJson(out);

    const auto &tall = map.at(GridCell{.column = 2, .row = 0});
    EXPECT_EQ(tall.top()->overlay, Overlay::Bridge);
    EXPECT_EQ(tall.slabAt(0)->overlay, Overlay::None);
    EXPECT_EQ(tall.slabAt(1)->overlay, Overlay::None);
    EXPECT_EQ(tall.top()->light, 100);
    EXPECT_EQ(tall.slabAt(0)->light, 255);

    const auto &water =
        map.at(GridCell{.column = 1, .row = 0}).top()->water;
    EXPECT_TRUE(water.deadly);
    EXPECT_TRUE(water.swimmable);
    EXPECT_EQ(water.current, FlowDirection::East);
}

TEST(MapJsonTest,
     TileMapFromJson_DefaultsALegacyEntityLevelToItsColumnHeight)
{
    auto out = legacyDocumentOf(3);

    nlohmann::json npc;
    npc["kind"] = "npc";
    npc["id"] = "keeper";
    npc["at"] = nlohmann::json::array({2, 0});
    out["entities"] = nlohmann::json::array({npc});

    const auto map = tileMapFromJson(out);

    EXPECT_EQ(std::get<Npc>(map.entities().front()).level, 2);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALevelsMemberThatIsNotAnArray)
{
    auto out = toJson(documentOf());

    out["levels"] = 5;

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsUnsortedLevels)
{
    TileMap map(MapHeader{.id = "two"}, 1, 1);

    (void)map.at(GridCell{.column = 0, .row = 0})
        .place(Slab{.level = 1});

    auto out = toJson(map);
    out["levels"] = nlohmann::json::array(
        {out["levels"].at(1), out["levels"].at(0)});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsARepeatedLevel)
{
    auto out = toJson(TileMap(MapHeader{.id = "twice"}, 1, 1));

    out["levels"] = nlohmann::json::array(
        {out["levels"].at(0), out["levels"].at(0)});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAnUnknownMarkInALevelRow)
{
    auto out = toJson(TileMap(MapHeader{.id = "marks"}, 1, 1));

    out["levels"].at(0)["terrain"] = nlohmann::json::array({"x"});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALevelRowLengthMismatch)
{
    auto out = toJson(TileMap(MapHeader{.id = "short"}, 1, 1));

    out["levels"].at(0)["terrain"] = nlohmann::json::array({"ff"});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALevelRowThatIsNotAString)
{
    auto out = toJson(TileMap(MapHeader{.id = "rows"}, 1, 1));

    out["levels"].at(0)["terrain"] = nlohmann::json::array({7});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALightEntryNamingNoSlab)
{
    TileMap map(MapHeader{.id = "lights"}, 2, 1);

    map.at(GridCell{.column = 1, .row = 0}).clear();

    auto out = toJson(map);
    out["levels"].at(0)["light"] = nlohmann::json::array(
        {nlohmann::json::array({1, 0, 10})});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALightEntryThatIsNotATriple)
{
    auto out = toJson(TileMap(MapHeader{.id = "lights"}, 1, 1));

    out["levels"].at(0)["light"] = nlohmann::json::array(
        {nlohmann::json::array({0, 0})});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAFullLightEntry)
{
    auto out = toJson(TileMap(MapHeader{.id = "lights"}, 1, 1));

    out["levels"].at(0)["light"] = nlohmann::json::array(
        {nlohmann::json::array({0, 0, 255})});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsABridgeNamingNoSlab)
{
    TileMap map(MapHeader{.id = "bridges"}, 2, 1);

    map.at(GridCell{.column = 1, .row = 0}).clear();

    auto out = toJson(map);
    out["levels"].at(0)["bridges"] = nlohmann::json::array(
        {nlohmann::json::array({1, 0})});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAWaterEntryNamingNoSlab)
{
    TileMap map(MapHeader{.id = "waters"}, 2, 1);

    map.at(GridCell{.column = 1, .row = 0}).clear();

    nlohmann::json entry;
    entry["at"] = nlohmann::json::array({1, 0});
    entry["deadly"] = false;
    entry["swimmable"] = true;

    auto out = toJson(map);
    out["levels"].at(0)["water"] = nlohmann::json::array({entry});

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAVersionFourEntityLackingALevel)
{
    TileMap map(MapHeader{.id = "entity"}, 1, 1);

    map.addEntity(Npc{.id = "keeper"});

    auto out = toJson(map);
    out["entities"].at(0).erase("level");

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALegacyHeightSpanBeyondTheBound)
{
    auto out = legacyDocumentOf(3);

    out["height"] = {{0, 0, 65}, {0, 0, 0}};

    EXPECT_THROW((void)tileMapFromJson(out), TileMapError);
}

namespace
{
    [[nodiscard]] TileMap richMap()
    {
        TileMap map(MapHeader{.id = "rich"}, 3, 2);

        auto *wet = map.at(GridCell{.column = 1, .row = 0}).top();
        wet->terrain = TerrainClass::Water;
        wet->light = 200;
        wet->water = antwika::tilemap::WaterAttributes{
            .deadly = true,
            .swimmable = true,
            .current = FlowDirection::East};

        map.at(GridCell{.column = 2, .row = 0}).top()->overlay =
            Overlay::Bridge;

        map.addEntity(Transition{
            .id = "door",
            .at = GridCell{.column = 0, .row = 0},
            .level = 0,
            .targetMap = "next",
            .targetEntry = "back",
            .requiredTags = {"key"}});
        map.addEntity(TriggerVolume{
            .id = "zone",
            .at = GridCell{.column = 0, .row = 0},
            .level = 0,
            .columns = 1,
            .rows = 1,
            .event = "step",
            .grantedTags = {"wet"}});

        return map;
    }

    [[nodiscard]] nlohmann::json validDocument()
    {
        return toJson(richMap());
    }
}

TEST(MapJsonTest, ToJson_MarksEveryTerrainClass)
{
    TileMap map(MapHeader{.id = "marks"}, 6, 1);
    const TerrainClass order[] = {
        TerrainClass::Floor,
        TerrainClass::Wall,
        TerrainClass::Water,
        TerrainClass::Cliff,
        TerrainClass::Path,
        TerrainClass::Stair};

    for (std::uint32_t column = 0; column < 6; ++column)
    {
        map.at(GridCell{.column = column, .row = 0})
            .top()
            ->terrain = order[column];
    }

    const auto out = toJson(map);

    EXPECT_EQ(out["levels"][0]["terrain"][0], "fw~cps");
}

TEST(MapJsonTest, ToJson_MarksAnUnknownTerrainWithAQuestionMark)
{
    TileMap map(MapHeader{.id = "odd"}, 1, 1);
    map.at(GridCell{}).top()->terrain =
        static_cast<TerrainClass>(42);

    const auto out = toJson(map);

    EXPECT_EQ(out["levels"][0]["terrain"][0], "?");
}

TEST(MapJsonTest, TileMapFromJson_RejectsAnEntityThatIsNotAnObject)
{
    auto document = validDocument();
    document["entities"][0] = 42;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAMemberThatIsNotAnInteger)
{
    auto document = validDocument();
    document["columns"] = "three";

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAnUnsignedBeyondTheSignedTop)
{
    auto document = validDocument();
    document["columns"] =
        std::numeric_limits<std::uint64_t>::max();

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAMemberBelowItsMinimum)
{
    auto document = validDocument();
    document["columns"] = 0;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAMemberThatIsNotAString)
{
    auto document = validDocument();
    document["id"] = 42;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAMemberThatIsNotABoolean)
{
    auto document = validDocument();
    document["levels"][0]["water"][0]["deadly"] = "yes";

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAMemberThatIsNotAnArray)
{
    auto document = validDocument();
    document["levels"][0]["light"] = 5;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAColorWithoutThreeChannels)
{
    auto document = validDocument();
    document["ink"] = nlohmann::json::array({1, 2});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsATagListThatIsNotAnArray)
{
    auto document = validDocument();
    document["entities"][0]["requiredTags"] = 5;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsATagListHoldingANonString)
{
    auto document = validDocument();
    document["entities"][0]["requiredTags"] =
        nlohmann::json::array({5});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsACellRefThatIsNotAPair)
{
    auto document = validDocument();
    document["entities"][0]["at"] = nlohmann::json::array({1});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAnEntityOfAnUnknownKind)
{
    auto document = validDocument();
    document["entities"][0]["kind"] = "dragon";

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALightEntryOfTheWrongShape)
{
    auto document = validDocument();
    document["levels"][0]["light"] =
        nlohmann::json::array({nlohmann::json::array({1, 2})});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsATriggerReachingPastTheEast)
{
    auto document = validDocument();
    document["entities"][1]["at"] = nlohmann::json::array({2, 0});
    document["entities"][1]["columns"] = 2;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsATriggerReachingPastTheSouth)
{
    auto document = validDocument();
    document["entities"][1]["at"] = nlohmann::json::array({0, 1});
    document["entities"][1]["rows"] = 2;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_TakesATriggerThatFitsTheGrid)
{
    auto document = validDocument();
    document["entities"][1]["at"] = nlohmann::json::array({1, 0});
    document["entities"][1]["columns"] = 2;
    document["entities"][1]["rows"] = 2;

    const auto map = tileMapFromJson(document);
    const auto &trigger =
        std::get<TriggerVolume>(map.entities().back());

    EXPECT_EQ(trigger.columns, 2U);
    EXPECT_EQ(trigger.rows, 2U);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsAFreeRowThatIsNotAString)
{
    auto document = validDocument();
    document["free"][0] = 5;

    EXPECT_THROW((void)mapDocumentFromJson(document), TileMapError);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsTooFewFreeRows)
{
    auto document = validDocument();
    document["free"] = nlohmann::json::array({"..."});

    EXPECT_THROW((void)mapDocumentFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALegacyRowOfTheWrongLength)
{
    auto document = legacyDocumentOf(3);
    document["height"] = nlohmann::json::array(
        {nlohmann::json::array({0, 0}),
         nlohmann::json::array({0, 0, 0})});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsADocumentThatIsNotAnObject)
{
    EXPECT_THROW(
        (void)tileMapFromJson(nlohmann::json(5)), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsAColorThatIsNotAnArray)
{
    auto document = validDocument();
    document["ink"] = 5;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsACellRefThatIsNotAnArray)
{
    auto document = validDocument();
    document["entities"][0]["at"] = 5;

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALightEntryThatIsNotAnArray)
{
    auto document = validDocument();
    document["levels"][0]["light"] = nlohmann::json::array({5});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, TileMapFromJson_RejectsALegacyRowThatIsNotAnArray)
{
    auto document = legacyDocumentOf(3);
    document["height"] = nlohmann::json::array({5, 5});

    EXPECT_THROW((void)tileMapFromJson(document), TileMapError);
}

TEST(MapJsonTest, MapDocumentFromJson_RejectsAFreeSectionThatIsNotAnArray)
{
    auto document = validDocument();
    document["free"] = 5;

    EXPECT_THROW((void)mapDocumentFromJson(document), TileMapError);
}
