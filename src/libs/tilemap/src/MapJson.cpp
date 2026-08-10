#include "antwika/tilemap/MapJson.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/enums/FromName.hpp>
#include <antwika/enums/NameTable.hpp>
#include <antwika/geometry/Grid.hpp>

#include "antwika/tilemap/Cell.hpp"
#include "antwika/tilemap/Entities.hpp"
#include "antwika/tilemap/FlowDirection.hpp"
#include "antwika/tilemap/MapHeader.hpp"
#include "antwika/tilemap/Overlay.hpp"
#include "antwika/tilemap/Rgb.hpp"
#include "antwika/tilemap/TerrainClass.hpp"
#include "antwika/tilemap/TileMapError.hpp"
#include "antwika/tilemap/WaterAttributes.hpp"

namespace antwika::tilemap
{

    namespace
    {
        using nlohmann::json;

        constexpr enums::NameTable<FlowDirection> kFlowNames{
            {"north", "east", "south", "west"}};

        [[nodiscard]] char terrainMark(const TerrainClass terrain)
        {
            switch (terrain)
            {
                case TerrainClass::Floor:
                    return 'f';
                case TerrainClass::Wall:
                    return 'w';
                case TerrainClass::Water:
                    return '~';
                case TerrainClass::Cliff:
                    return 'c';
                case TerrainClass::Path:
                    return 'p';
                case TerrainClass::Stair:
                    return 's';
            }

            return '?';
        }

        [[nodiscard]] TerrainClass terrainFromMark(const char mark)
        {
            switch (mark)
            {
                case 'f':
                    return TerrainClass::Floor;
                case 'w':
                    return TerrainClass::Wall;
                case '~':
                    return TerrainClass::Water;
                case 'c':
                    return TerrainClass::Cliff;
                case 'p':
                    return TerrainClass::Path;
                case 's':
                    return TerrainClass::Stair;
                default:
                    throw TileMapError(
                        std::string(
                            "the terrain row holds an unknown mark: ")
                        + mark);
            }
        }

        [[nodiscard]] json pairJson(const geometry::GridCell cell)
        {
            return json::array({cell.column, cell.row});
        }

        [[nodiscard]] json rgbJson(const Rgb color)
        {
            return json::array({color.red, color.green, color.blue});
        }

        [[nodiscard]] json terrainJson(const TileMap &map)
        {
            auto rows = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                std::string line;
                line.reserve(map.columns());

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    line.push_back(terrainMark(
                        map.at({.column = column, .row = row})
                            .terrain));
                }

                rows.push_back(std::move(line));
            }

            return rows;
        }

        template <typename Pick>
        [[nodiscard]] json numberRowsJson(
            const TileMap &map, Pick pick)
        {
            auto rows = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                auto line = json::array();

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    line.push_back(
                        pick(map.at({.column = column, .row = row})));
                }

                rows.push_back(std::move(line));
            }

            return rows;
        }

        [[nodiscard]] json bridgesJson(const TileMap &map)
        {
            auto entries = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const geometry::GridCell at{
                        .column = column, .row = row};

                    if (map.at(at).overlay == Overlay::Bridge)
                    {
                        entries.push_back(pairJson(at));
                    }
                }
            }

            return entries;
        }

        [[nodiscard]] json waterJson(
            const geometry::GridCell at, const WaterAttributes &water)
        {
            json entry;
            entry["at"] = pairJson(at);
            entry["deadly"] = water.deadly;
            entry["swimmable"] = water.swimmable;

            if (water.current.has_value())
            {
                entry["current"] =
                    std::string(toString(*water.current));
            }

            return entry;
        }

        [[nodiscard]] json watersJson(const TileMap &map)
        {
            auto entries = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const geometry::GridCell at{
                        .column = column, .row = row};
                    const auto &water = map.at(at).water;

                    if (water != WaterAttributes{})
                    {
                        entries.push_back(waterJson(at, water));
                    }
                }
            }

            return entries;
        }

        [[nodiscard]] json entityJson(const Transition &transition)
        {
            json entry;
            entry["kind"] = "transition";
            entry["id"] = transition.id;
            entry["at"] = pairJson(transition.at);
            entry["targetMap"] = transition.targetMap;
            entry["targetEntry"] = transition.targetEntry;
            entry["requiredTags"] = transition.requiredTags;
            return entry;
        }

        [[nodiscard]] json entityJson(const BoatEmbark &boat)
        {
            json entry;
            entry["kind"] = "boat";
            entry["id"] = boat.id;
            entry["at"] = pairJson(boat.at);
            return entry;
        }

        [[nodiscard]] json entityJson(const SpawnPoint &spawn)
        {
            json entry;
            entry["kind"] = "spawn";
            entry["id"] = spawn.id;
            entry["at"] = pairJson(spawn.at);
            entry["enemy"] = spawn.enemy;
            return entry;
        }

        [[nodiscard]] json entityJson(const Pickup &pickup)
        {
            json entry;
            entry["kind"] = "pickup";
            entry["id"] = pickup.id;
            entry["at"] = pairJson(pickup.at);
            entry["item"] = pickup.item;
            entry["grantedTags"] = pickup.grantedTags;
            return entry;
        }

        [[nodiscard]] json entityJson(const Npc &npc)
        {
            json entry;
            entry["kind"] = "npc";
            entry["id"] = npc.id;
            entry["at"] = pairJson(npc.at);
            return entry;
        }

        [[nodiscard]] json entityJson(const TriggerVolume &trigger)
        {
            json entry;
            entry["kind"] = "trigger";
            entry["id"] = trigger.id;
            entry["at"] = pairJson(trigger.at);
            entry["columns"] = trigger.columns;
            entry["rows"] = trigger.rows;
            entry["event"] = trigger.event;
            entry["grantedTags"] = trigger.grantedTags;
            return entry;
        }

        [[nodiscard]] json entitiesJson(const TileMap &map)
        {
            auto entries = json::array();

            for (const auto &entity : map.entities())
            {
                entries.push_back(std::visit(
                    [](const auto &value)
                    { return entityJson(value); },
                    entity));
            }

            return entries;
        }

        [[nodiscard]] const json &member(
            const json &object, const std::string &key)
        {
            if (!object.is_object())
            {
                throw TileMapError(
                    "the entry that should hold " + key
                    + " is not an object");
            }

            if (!object.contains(key))
            {
                throw TileMapError(
                    "the entry lacks the member: " + key);
            }

            return object.at(key);
        }

        [[nodiscard]] std::int64_t wholeValue(
            const json &value,
            const std::int64_t minimum,
            const std::int64_t maximum,
            const std::string &what)
        {
            if (!value.is_number_integer())
            {
                throw TileMapError(what + " is not an integer");
            }

            constexpr auto kTop =
                std::numeric_limits<std::int64_t>::max();

            if (value.is_number_unsigned()
                && value.get<std::uint64_t>()
                       > static_cast<std::uint64_t>(kTop))
            {
                throw TileMapError(what + " is out of range");
            }

            const auto number = value.get<std::int64_t>();

            if (number < minimum || number > maximum)
            {
                throw TileMapError(what + " is out of range");
            }

            return number;
        }

        [[nodiscard]] std::int64_t wholeMember(
            const json &object,
            const std::string &key,
            const std::int64_t minimum,
            const std::int64_t maximum)
        {
            return wholeValue(
                member(object, key), minimum, maximum,
                "the member " + key);
        }

        [[nodiscard]] std::string stringMember(
            const json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_string())
            {
                throw TileMapError(
                    "the member is not a string: " + key);
            }

            return value.get<std::string>();
        }

        [[nodiscard]] bool flagMember(
            const json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_boolean())
            {
                throw TileMapError(
                    "the member is not a boolean: " + key);
            }

            return value.get<bool>();
        }

        [[nodiscard]] Rgb rgbMember(
            const json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_array() || value.size() != 3)
            {
                throw TileMapError(
                    "the color does not hold three channels: " + key);
            }

            const auto channel =
                [&value, &key](const std::size_t index)
            {
                return static_cast<std::uint8_t>(wholeValue(
                    value.at(index), 0, 255,
                    "the color channel in " + key));
            };

            return Rgb{
                .red = channel(0),
                .green = channel(1),
                .blue = channel(2)};
        }

        [[nodiscard]] std::vector<std::string> tagsMember(
            const json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_array())
            {
                throw TileMapError(
                    "the member is not an array: " + key);
            }

            std::vector<std::string> tags;
            tags.reserve(value.size());

            for (const auto &tag : value)
            {
                if (!tag.is_string())
                {
                    throw TileMapError(
                        "the tag list holds a non-string: " + key);
                }

                tags.push_back(tag.get<std::string>());
            }

            return tags;
        }

        [[nodiscard]] geometry::GridCell cellRef(
            const json &value,
            const TileMap &map,
            const std::string &what)
        {
            if (!value.is_array() || value.size() != 2)
            {
                throw TileMapError(
                    what + " does not hold a column and row pair");
            }

            const auto column = wholeValue(
                value.at(0), 0, map.columns() - 1, what + " column");
            const auto row = wholeValue(
                value.at(1), 0, map.rows() - 1, what + " row");

            return geometry::GridCell{
                .column = static_cast<std::uint32_t>(column),
                .row = static_cast<std::uint32_t>(row)};
        }

        [[nodiscard]] const json &rowsMember(
            const json &document,
            const std::string &key,
            const std::uint32_t rows)
        {
            const auto &value = member(document, key);

            if (!value.is_array() || value.size() != rows)
            {
                throw TileMapError(
                    "the member does not hold one entry per row: "
                    + key);
            }

            return value;
        }

        void decodeTerrain(const json &document, TileMap &map)
        {
            const std::string key = "terrain";
            const auto &rows = rowsMember(document, key, map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                const auto &line = rows.at(row);

                if (!line.is_string())
                {
                    throw TileMapError(
                        "the terrain row is not a string");
                }

                const auto marks = line.get<std::string>();

                if (marks.size() != map.columns())
                {
                    throw TileMapError(
                        "the terrain row length differs from the "
                        "column count");
                }

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    map.at({.column = column, .row = row}).terrain =
                        terrainFromMark(marks[column]);
                }
            }
        }

        template <typename Apply>
        void decodeNumberRows(
            const json &document,
            TileMap &map,
            const std::string &key,
            const std::int64_t minimum,
            const std::int64_t maximum,
            Apply apply)
        {
            const auto &rows = rowsMember(document, key, map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                const auto &line = rows.at(row);

                if (!line.is_array()
                    || line.size() != map.columns())
                {
                    throw TileMapError(
                        "the row does not hold one value per "
                        "column: "
                        + key);
                }

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    apply(
                        map.at({.column = column, .row = row}),
                        wholeValue(
                            line.at(column), minimum, maximum,
                            "the " + key + " value"));
                }
            }
        }

        void decodeBridges(const json &document, TileMap &map)
        {
            const std::string key = "bridges";
            const auto &entries = member(document, key);

            if (!entries.is_array())
            {
                throw TileMapError(
                    "the member is not an array: bridges");
            }

            for (const auto &entry : entries)
            {
                map.at(cellRef(entry, map, "the bridge")).overlay =
                    Overlay::Bridge;
            }
        }

        void decodeWaters(const json &document, TileMap &map)
        {
            const std::string key = "water";
            const auto &entries = member(document, key);

            if (!entries.is_array())
            {
                throw TileMapError(
                    "the member is not an array: water");
            }

            for (const auto &entry : entries)
            {
                const auto at = cellRef(
                    member(entry, "at"), map, "the water cell");

                auto &water = map.at(at).water;
                water.deadly = flagMember(entry, "deadly");
                water.swimmable = flagMember(entry, "swimmable");

                if (entry.contains("current"))
                {
                    water.current = enums::fromName<TileMapError>(
                        kFlowNames,
                        stringMember(entry, "current"),
                        "the water current names an unknown "
                        "direction: ");
                }
            }
        }

        [[nodiscard]] Entity transitionFrom(
            const json &entry, const TileMap &map)
        {
            return Transition{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity"),
                .targetMap = stringMember(entry, "targetMap"),
                .targetEntry = stringMember(entry, "targetEntry"),
                .requiredTags = tagsMember(entry, "requiredTags")};
        }

        [[nodiscard]] Entity boatFrom(
            const json &entry, const TileMap &map)
        {
            return BoatEmbark{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity")};
        }

        [[nodiscard]] Entity spawnFrom(
            const json &entry, const TileMap &map)
        {
            return SpawnPoint{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity"),
                .enemy = stringMember(entry, "enemy")};
        }

        [[nodiscard]] Entity pickupFrom(
            const json &entry, const TileMap &map)
        {
            return Pickup{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity"),
                .item = stringMember(entry, "item"),
                .grantedTags = tagsMember(entry, "grantedTags")};
        }

        [[nodiscard]] Entity npcFrom(
            const json &entry, const TileMap &map)
        {
            return Npc{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity")};
        }

        [[nodiscard]] Entity triggerFrom(
            const json &entry, const TileMap &map)
        {
            const TriggerVolume trigger{
                .id = stringMember(entry, "id"),
                .at = cellRef(
                    member(entry, "at"), map, "the entity"),
                .columns = static_cast<std::uint32_t>(wholeMember(
                    entry, "columns", 1, map.columns())),
                .rows = static_cast<std::uint32_t>(wholeMember(
                    entry, "rows", 1, map.rows())),
                .event = stringMember(entry, "event"),
                .grantedTags = tagsMember(entry, "grantedTags")};

            const auto right =
                std::uint64_t{trigger.at.column} + trigger.columns;
            const auto bottom =
                std::uint64_t{trigger.at.row} + trigger.rows;

            if (right > map.columns() || bottom > map.rows())
            {
                throw TileMapError(
                    "the trigger volume reaches outside the grid");
            }

            return trigger;
        }

        [[nodiscard]] Entity entityFrom(
            const json &entry, const TileMap &map)
        {
            const auto kind = stringMember(entry, "kind");

            if (kind == "transition")
            {
                return transitionFrom(entry, map);
            }

            if (kind == "boat")
            {
                return boatFrom(entry, map);
            }

            if (kind == "spawn")
            {
                return spawnFrom(entry, map);
            }

            if (kind == "pickup")
            {
                return pickupFrom(entry, map);
            }

            if (kind == "npc")
            {
                return npcFrom(entry, map);
            }

            if (kind == "trigger")
            {
                return triggerFrom(entry, map);
            }

            throw TileMapError(
                "the entity names an unknown kind: " + kind);
        }

        void decodeEntities(const json &document, TileMap &map)
        {
            const std::string key = "entities";
            const auto &entries = member(document, key);

            if (!entries.is_array())
            {
                throw TileMapError(
                    "the member is not an array: entities");
            }

            for (const auto &entry : entries)
            {
                map.addEntity(entityFrom(entry, map));
            }
        }

        constexpr char kPinnedMark = '.';

        constexpr char kFreeMark = 'o';

        [[nodiscard]] json freeJson(
            const TileMap &map, const std::vector<bool> &free)
        {
            auto rows = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                std::string line;
                line.reserve(map.columns());

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto index =
                        static_cast<std::size_t>(row)
                            * map.columns()
                        + column;
                    const bool cellFree =
                        index < free.size() && free[index];

                    line.push_back(
                        cellFree ? kFreeMark : kPinnedMark);
                }

                rows.push_back(std::move(line));
            }

            return rows;
        }

        [[nodiscard]] std::vector<bool> decodeFree(
            const json &document, const TileMap &map)
        {
            std::vector<bool> free(
                static_cast<std::size_t>(map.columns())
                    * map.rows(),
                false);

            if (!document.contains("free"))
            {
                return free;
            }

            const std::string key = "free";
            const auto &rows = rowsMember(document, key, map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                const auto &line = rows.at(row);

                if (!line.is_string())
                {
                    throw TileMapError(
                        "the free row is not a string");
                }

                const auto marks = line.get<std::string>();

                if (marks.size() != map.columns())
                {
                    throw TileMapError(
                        "the free row length differs from the "
                        "column count");
                }

                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const char mark = marks[column];

                    if (mark != kPinnedMark && mark != kFreeMark)
                    {
                        throw TileMapError(
                            std::string(
                                "the free row holds an unknown "
                                "mark: ")
                            + mark);
                    }

                    free[static_cast<std::size_t>(row)
                             * map.columns()
                         + column] = mark == kFreeMark;
                }
            }

            return free;
        }
    }

    nlohmann::json toJson(const TileMap &map)
    {
        json out;
        out["schema"] = kSchemaVersion;
        out["id"] = map.header().id;
        out["ink"] = rgbJson(map.header().ink);
        out["paper"] = rgbJson(map.header().paper);
        out["columns"] = map.columns();
        out["rows"] = map.rows();
        out["terrain"] = terrainJson(map);
        out["height"] = numberRowsJson(
            map, [](const Cell &cell) { return cell.height; });
        out["light"] = numberRowsJson(
            map, [](const Cell &cell) { return cell.light; });
        out["bridges"] = bridgesJson(map);
        out["water"] = watersJson(map);
        out["entities"] = entitiesJson(map);
        out["free"] = freeJson(map, {});
        return out;
    }

    nlohmann::json toJson(const MapDocument &document)
    {
        auto out = toJson(document.map);
        out["free"] = freeJson(document.map, document.free);
        return out;
    }

    TileMap tileMapFromJson(const nlohmann::json &document)
    {
        if (!document.is_object())
        {
            throw TileMapError("the map document is not an object");
        }

        const auto schema = wholeMember(
            document, "schema", 0,
            std::numeric_limits<std::int64_t>::max());

        if (schema != 1 && schema != kSchemaVersion)
        {
            throw TileMapError(
                "the map names a schema version this build does not "
                "know: "
                + std::to_string(schema));
        }

        TileMap map(
            MapHeader{
                .id = stringMember(document, "id"),
                .schemaVersion = static_cast<std::uint32_t>(schema),
                .ink = rgbMember(document, "ink"),
                .paper = rgbMember(document, "paper")},
            static_cast<std::uint32_t>(wholeMember(
                document, "columns", 1,
                std::numeric_limits<std::uint32_t>::max())),
            static_cast<std::uint32_t>(wholeMember(
                document, "rows", 1,
                std::numeric_limits<std::uint32_t>::max())));

        decodeTerrain(document, map);
        decodeNumberRows(
            document, map, "height",
            std::numeric_limits<std::int32_t>::min(),
            std::numeric_limits<std::int32_t>::max(),
            [](Cell &cell, const std::int64_t value)
            { cell.height = static_cast<std::int32_t>(value); });
        decodeNumberRows(
            document, map, "light", 0, 255,
            [](Cell &cell, const std::int64_t value)
            { cell.light = static_cast<std::uint8_t>(value); });
        decodeBridges(document, map);
        decodeWaters(document, map);
        decodeEntities(document, map);

        return map;
    }

    MapDocument mapDocumentFromJson(const nlohmann::json &document)
    {
        MapDocument loaded{.map = tileMapFromJson(document)};

        const auto schema = wholeMember(
            document, "schema", 0,
            std::numeric_limits<std::int64_t>::max());

        if (schema >= 2 && !document.contains("free"))
        {
            throw TileMapError(
                "the version 2 map lacks the free section");
        }

        loaded.free = decodeFree(document, loaded.map);

        return loaded;
    }

}
