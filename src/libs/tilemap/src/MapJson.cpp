#include "antwika/tilemap/MapJson.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/enums/FromName.hpp>
#include <antwika/enums/NameTable.hpp>
#include <antwika/geometry/Grid.hpp>

#include "antwika/tilemap/Column.hpp"
#include "antwika/tilemap/Entities.hpp"
#include "antwika/tilemap/FlowDirection.hpp"
#include "antwika/tilemap/MapHeader.hpp"
#include "antwika/tilemap/Overlay.hpp"
#include "antwika/tilemap/Rgb.hpp"
#include "antwika/tilemap/Slab.hpp"
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

        constexpr char kNoSlabMark = '.';

        constexpr std::int64_t kMaxMigratedSpan = 64;

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

        [[nodiscard]] TerrainClass terrainNamed(
            const std::string &name)
        {
            for (const auto terrain : enums::kAll<TerrainClass>)
            {
                if (toString(terrain) == name)
                {
                    return terrain;
                }
            }

            throw TileMapError(
                "the tilesets entry names an unknown terrain: "
                + name);
        }

        [[nodiscard]] json tilesetsJson(const TileMap &map)
        {
            auto entries = json::object();

            for (const auto terrain : enums::kAll<TerrainClass>)
            {
                const auto &binding =
                    map.header().tilesets[enums::index(terrain)];

                if (!binding.empty())
                {
                    entries[std::string(toString(terrain))] =
                        binding;
                }
            }

            return entries;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json pairJson(const geometry::GridCell cell)
        {
            auto entry = json::array();
            entry.push_back(cell.column);
            entry.push_back(cell.row);

            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json rgbJson(const Rgb color)
        {
            auto entry = json::array();
            entry.push_back(color.red);
            entry.push_back(color.green);
            entry.push_back(color.blue);

            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json levelTerrainJson(
            const TileMap &map, const std::int32_t level)
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
                    const auto *slab =
                        map.at({.column = column, .row = row})
                            .slabAt(level);

                    line.push_back(
                        slab == nullptr
                            ? kNoSlabMark
                            : terrainMark(slab->terrain));
                }

                rows.push_back(std::move(line));
            }

            return rows;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json levelLightJson(
            const TileMap &map, const std::int32_t level)
        {
            auto entries = json::array();

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto *slab =
                        map.at({.column = column, .row = row})
                            .slabAt(level);

                    if (slab != nullptr && slab->light != 255)
                    {
                        auto item = json::array();
                        item.push_back(column);
                        item.push_back(row);
                        item.push_back(slab->light);
                        entries.push_back(std::move(item));
                    }
                }
            }

            return entries;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json levelBridgesJson(
            const TileMap &map, const std::int32_t level)
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
                    const auto *slab = map.at(at).slabAt(level);

                    if (slab != nullptr
                        && slab->overlay == Overlay::Bridge)
                    {
                        entries.push_back(pairJson(at));
                    }
                }
            }

            return entries;
        } // GCOVR_EXCL_LINE

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
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json levelWaterJson(
            const TileMap &map, const std::int32_t level)
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
                    const auto *slab = map.at(at).slabAt(level);

                    if (slab != nullptr
                        && slab->water != WaterAttributes{})
                    {
                        entries.push_back(
                            waterJson(at, slab->water));
                    }
                }
            }

            return entries;
        } // GCOVR_EXCL_LINE

        void addUnlessEmpty(
            json &entry, const std::string &key, json value)
        {
            if (!value.empty())
            {
                entry[key] = std::move(value);
            }
        }

        [[nodiscard]] json levelJson(
            const TileMap &map, const std::int32_t level)
        {
            json entry;
            entry["level"] = level;
            entry["terrain"] = levelTerrainJson(map, level);
            addUnlessEmpty(
                entry, "bridges", levelBridgesJson(map, level));
            addUnlessEmpty(
                entry, "light", levelLightJson(map, level));
            addUnlessEmpty(
                entry, "water", levelWaterJson(map, level));
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json levelsJson(const TileMap &map)
        {
            std::set<std::int32_t> levels;

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto &slabs =
                        map.at({.column = column, .row = row})
                            .slabs();

                    for (const auto &slab : slabs)
                    {
                        levels.insert(slab.level);
                    }
                }
            }

            auto entries = json::array();

            for (const auto level : levels)
            {
                entries.push_back(levelJson(map, level));
            }

            return entries;
        }

        [[nodiscard]] json entityJson(const Transition &transition)
        {
            json entry;
            entry["kind"] = "transition";
            entry["id"] = transition.id;
            entry["at"] = pairJson(transition.at);
            entry["level"] = transition.level;
            entry["targetMap"] = transition.targetMap;
            entry["targetEntry"] = transition.targetEntry;
            entry["requiredTags"] = transition.requiredTags;
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json entityJson(const BoatEmbark &boat)
        {
            json entry;
            entry["kind"] = "boat";
            entry["id"] = boat.id;
            entry["at"] = pairJson(boat.at);
            entry["level"] = boat.level;
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json entityJson(const SpawnPoint &spawn)
        {
            json entry;
            entry["kind"] = "spawn";
            entry["id"] = spawn.id;
            entry["at"] = pairJson(spawn.at);
            entry["level"] = spawn.level;
            entry["enemy"] = spawn.enemy;
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json entityJson(const Pickup &pickup)
        {
            json entry;
            entry["kind"] = "pickup";
            entry["id"] = pickup.id;
            entry["at"] = pairJson(pickup.at);
            entry["level"] = pickup.level;
            entry["item"] = pickup.item;
            entry["grantedTags"] = pickup.grantedTags;
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json entityJson(const Npc &npc)
        {
            json entry;
            entry["kind"] = "npc";
            entry["id"] = npc.id;
            entry["at"] = pairJson(npc.at);
            entry["level"] = npc.level;
            return entry;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] json entityJson(const TriggerVolume &trigger)
        {
            json entry;
            entry["kind"] = "trigger";
            entry["id"] = trigger.id;
            entry["at"] = pairJson(trigger.at);
            entry["level"] = trigger.level;
            entry["columns"] = trigger.columns;
            entry["rows"] = trigger.rows;
            entry["event"] = trigger.event;
            entry["grantedTags"] = trigger.grantedTags;
            return entry;
        } // GCOVR_EXCL_LINE

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
        } // GCOVR_EXCL_LINE

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

        [[nodiscard]] const json &arrayMember(
            const json &object, const std::string &key)
        {
            const auto &value = member(object, key);

            if (!value.is_array())
            {
                throw TileMapError(
                    "the member is not an array: " + key);
            }

            return value;
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

        [[nodiscard]] std::array<
            std::string, enums::kCount<TerrainClass>>
        decodeTilesets(
            const json &document, const std::int64_t schema)
        {
            std::array<std::string, enums::kCount<TerrainClass>>
                tilesets{};

            if (schema <= 2)
            {
                return tilesets;
            }

            const std::string key = "tilesets";
            const auto &entries = member(document, key);

            if (!entries.is_object())
            {
                throw TileMapError(
                    "the member is not an object: tilesets");
            }

            for (const auto &[name, value] : entries.items())
            {
                if (!value.is_string())
                {
                    throw TileMapError(
                        "the tileset binding is not a string: "
                        + name);
                }

                tilesets[enums::index(terrainNamed(name))] =
                    value.get<std::string>();
            }

            return tilesets;
        }

        [[nodiscard]] std::vector<std::string> terrainRows(
            const json &holder, const TileMap &map)
        {
            const std::string key = "terrain";
            const auto &rows = rowsMember(holder, key, map.rows());

            std::vector<std::string> lines;
            lines.reserve(map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                const auto &line = rows.at(row);

                if (!line.is_string())
                {
                    throw TileMapError(
                        "the terrain row is not a string");
                }

                auto marks = line.get<std::string>();

                if (marks.size() != map.columns())
                {
                    throw TileMapError(
                        "the terrain row length differs from the "
                        "column count");
                }

                lines.push_back(std::move(marks));
            }

            return lines;
        }

        [[nodiscard]] std::vector<TerrainClass> terrainGrid(
            const json &document, const TileMap &map)
        {
            const auto lines = terrainRows(document, map);

            std::vector<TerrainClass> grid;
            grid.reserve(
                static_cast<std::size_t>(map.rows())
                * map.columns());

            for (const auto &line : lines)
            {
                for (const char mark : line)
                {
                    grid.push_back(terrainFromMark(mark));
                }
            }

            return grid;
        }

        [[nodiscard]] std::vector<std::int64_t> numberGrid(
            const json &document,
            const TileMap &map,
            const std::string &key,
            const std::int64_t minimum,
            const std::int64_t maximum)
        {
            const auto &rows = rowsMember(document, key, map.rows());

            std::vector<std::int64_t> grid;
            grid.reserve(
                static_cast<std::size_t>(map.rows())
                * map.columns());

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
                    grid.push_back(wholeValue(
                        line.at(column), minimum, maximum,
                        "the " + key + " value"));
                }
            }

            return grid;
        }

        void applyWater(const json &entry, WaterAttributes &water)
        {
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

        void migrateLegacyColumns(const json &document, TileMap &map)
        {
            const auto terrain = terrainGrid(document, map);
            const auto heights = numberGrid(
                document, map, "height",
                std::numeric_limits<std::int32_t>::min(),
                std::numeric_limits<std::int32_t>::max());
            const auto lights =
                numberGrid(document, map, "light", 0, 255);

            std::int64_t base = 0;
            std::int64_t highest = heights.front();

            for (const auto height : heights)
            {
                base = std::min(base, height);
                highest = std::max(highest, height);
            }

            if (highest - base > kMaxMigratedSpan)
            {
                throw TileMapError(
                    "the legacy height span is too large to "
                    "migrate");
            }

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto index =
                        static_cast<std::size_t>(row)
                            * map.columns()
                        + column;
                    auto &stack =
                        map.at({.column = column, .row = row});

                    stack.clear();

                    for (auto level = base;
                         level <= heights[index];
                         ++level)
                    {
                        (void)stack.place(Slab{
                            .level =
                                static_cast<std::int32_t>(level),
                            .terrain = terrain[index]});
                    }

                    stack.top()->light =
                        static_cast<std::uint8_t>(lights[index]);
                }
            }
        }

        void decodeLegacyBridges(const json &document, TileMap &map)
        {
            const std::string key = "bridges";

            for (const auto &entry : arrayMember(document, key))
            {
                map.at(cellRef(entry, map, "the bridge"))
                    .top()
                    ->overlay = Overlay::Bridge;
            }
        }

        void decodeLegacyWaters(const json &document, TileMap &map)
        {
            const std::string key = "water";

            for (const auto &entry : arrayMember(document, key))
            {
                const auto at = cellRef(
                    member(entry, "at"), map, "the water cell");

                applyWater(entry, map.at(at).top()->water);
            }
        }

        void decodeLevelTerrain(
            const json &entry,
            TileMap &map,
            const std::int32_t level)
        {
            const auto lines = terrainRows(entry, map);

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const char mark = lines[row][column];

                    if (mark != kNoSlabMark)
                    {
                        (void)map
                            .at({.column = column, .row = row})
                            .place(Slab{
                                .level = level,
                                .terrain = terrainFromMark(mark)});
                    }
                }
            }
        }

        void decodeLevelLight(
            const json &entry,
            TileMap &map,
            const std::int32_t level)
        {
            const std::string key = "light";

            if (!entry.contains(key))
            {
                return;
            }

            for (const auto &item : arrayMember(entry, key))
            {
                if (!item.is_array() || item.size() != 3)
                {
                    throw TileMapError(
                        "the light entry does not hold a column, "
                        "row and value");
                }

                const auto column =
                    static_cast<std::uint32_t>(wholeValue(
                        item.at(0), 0, map.columns() - 1,
                        "the light entry column"));
                const auto row =
                    static_cast<std::uint32_t>(wholeValue(
                        item.at(1), 0, map.rows() - 1,
                        "the light entry row"));
                const auto value = wholeValue(
                    item.at(2), 0, 254, "the light value");

                auto *slab =
                    map.at({.column = column, .row = row})
                        .slabAt(level);

                if (slab == nullptr)
                {
                    throw TileMapError(
                        "the light entry names a cell with no "
                        "slab");
                }

                slab->light = static_cast<std::uint8_t>(value);
            }
        }

        void decodeLevelBridges(
            const json &entry,
            TileMap &map,
            const std::int32_t level)
        {
            const std::string key = "bridges";

            if (!entry.contains(key))
            {
                return;
            }

            for (const auto &item : arrayMember(entry, key))
            {
                auto *slab =
                    map.at(cellRef(item, map, "the bridge"))
                        .slabAt(level);

                if (slab == nullptr)
                {
                    throw TileMapError(
                        "the bridge names a cell with no slab");
                }

                slab->overlay = Overlay::Bridge;
            }
        }

        void decodeLevelWater(
            const json &entry,
            TileMap &map,
            const std::int32_t level)
        {
            const std::string key = "water";

            if (!entry.contains(key))
            {
                return;
            }

            for (const auto &item : arrayMember(entry, key))
            {
                const auto at = cellRef(
                    member(item, "at"), map, "the water cell");

                auto *slab = map.at(at).slabAt(level);

                if (slab == nullptr)
                {
                    throw TileMapError(
                        "the water entry names a cell with no "
                        "slab");
                }

                applyWater(item, slab->water);
            }
        }

        void clearColumns(TileMap &map)
        {
            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    map.at({.column = column, .row = row}).clear();
                }
            }
        }

        void decodeLevels(const json &document, TileMap &map)
        {
            const std::string key = "levels";
            const auto &entries = arrayMember(document, key);

            clearColumns(map);

            bool decoded = false;
            std::int32_t previous = 0;

            for (const auto &entry : entries)
            {
                const auto level =
                    static_cast<std::int32_t>(wholeMember(
                        entry, "level",
                        std::numeric_limits<std::int32_t>::min(),
                        std::numeric_limits<std::int32_t>::max()));

                if (decoded && level <= previous)
                {
                    throw TileMapError("the levels do not ascend");
                }

                decoded = true;
                previous = level;

                decodeLevelTerrain(entry, map, level);
                decodeLevelLight(entry, map, level);
                decodeLevelBridges(entry, map, level);
                decodeLevelWater(entry, map, level);
            }
        }

        [[nodiscard]] std::int32_t entityLevel(
            const json &entry,
            const TileMap &map,
            const geometry::GridCell at,
            const std::int64_t schema)
        {
            if (schema == kSchemaVersion)
            {
                return static_cast<std::int32_t>(wholeMember(
                    entry, "level",
                    std::numeric_limits<std::int32_t>::min(),
                    std::numeric_limits<std::int32_t>::max()));
            }

            return map.at(at).top()->level;
        }

        [[nodiscard]] Entity transitionFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            Transition made;
            made.id = stringMember(entry, "id");
            made.at = at;
            made.level = entityLevel(entry, map, at, schema);
            made.targetMap = stringMember(entry, "targetMap");
            made.targetEntry = stringMember(entry, "targetEntry");
            made.requiredTags = tagsMember(entry, "requiredTags");

            return made;
        }

        [[nodiscard]] Entity boatFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            BoatEmbark made;
            made.id = stringMember(entry, "id");
            made.at = at;
            made.level = entityLevel(entry, map, at, schema);

            return made;
        }

        [[nodiscard]] Entity spawnFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            SpawnPoint made;
            made.id = stringMember(entry, "id");
            made.at = at;
            made.level = entityLevel(entry, map, at, schema);
            made.enemy = stringMember(entry, "enemy");

            return made;
        }

        [[nodiscard]] Entity pickupFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            Pickup made;
            made.id = stringMember(entry, "id");
            made.at = at;
            made.level = entityLevel(entry, map, at, schema);
            made.item = stringMember(entry, "item");
            made.grantedTags = tagsMember(entry, "grantedTags");

            return made;
        }

        [[nodiscard]] Entity npcFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            Npc made;
            made.id = stringMember(entry, "id");
            made.at = at;
            made.level = entityLevel(entry, map, at, schema);

            return made;
        }

        [[nodiscard]] Entity triggerFrom(
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto at =
                cellRef(member(entry, "at"), map, "the entity");

            TriggerVolume trigger;
            trigger.id = stringMember(entry, "id");
            trigger.at = at;
            trigger.level = entityLevel(entry, map, at, schema);
            trigger.columns = static_cast<std::uint32_t>(
                wholeMember(entry, "columns", 1, map.columns()));
            trigger.rows = static_cast<std::uint32_t>(
                wholeMember(entry, "rows", 1, map.rows()));
            trigger.event = stringMember(entry, "event");
            trigger.grantedTags = tagsMember(entry, "grantedTags");

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
            const json &entry,
            const TileMap &map,
            const std::int64_t schema)
        {
            const auto kind = stringMember(entry, "kind");

            if (kind == "transition")
            {
                return transitionFrom(entry, map, schema);
            }

            if (kind == "boat")
            {
                return boatFrom(entry, map, schema);
            }

            if (kind == "spawn")
            {
                return spawnFrom(entry, map, schema);
            }

            if (kind == "pickup")
            {
                return pickupFrom(entry, map, schema);
            }

            if (kind == "npc")
            {
                return npcFrom(entry, map, schema);
            }

            if (kind == "trigger")
            {
                return triggerFrom(entry, map, schema);
            }

            throw TileMapError(
                "the entity names an unknown kind: " + kind);
        }

        void decodeEntities(
            const json &document,
            TileMap &map,
            const std::int64_t schema)
        {
            const std::string key = "entities";

            for (const auto &entry : arrayMember(document, key))
            {
                map.addEntity(entityFrom(entry, map, schema));
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
        } // GCOVR_EXCL_LINE

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
        out["levels"] = levelsJson(map);
        out["entities"] = entitiesJson(map);
        out["tilesets"] = tilesetsJson(map);
        out["free"] = freeJson(map, {});
        return out;
    } // GCOVR_EXCL_LINE

    nlohmann::json toJson(const MapDocument &document)
    {
        auto out = toJson(document.map);
        out["free"] = freeJson(document.map, document.free);
        return out;
    } // GCOVR_EXCL_LINE

    TileMap tileMapFromJson(const nlohmann::json &document)
    {
        if (!document.is_object())
        {
            throw TileMapError("the map document is not an object");
        }

        const auto schema = wholeMember(
            document, "schema", 0,
            std::numeric_limits<std::int64_t>::max());

        if (schema != 1 && schema != 2 && schema != 3
            && schema != kSchemaVersion)
        {
            throw TileMapError(
                "the map names a schema version this build does not "
                "know: "
                + std::to_string(schema));
        }

        MapHeader header;
        header.id = stringMember(document, "id");
        header.schemaVersion = static_cast<std::uint32_t>(schema);
        header.ink = rgbMember(document, "ink");
        header.paper = rgbMember(document, "paper");
        header.tilesets = decodeTilesets(document, schema);

        TileMap map(
            std::move(header),
            static_cast<std::uint32_t>(wholeMember(
                document, "columns", 1,
                std::numeric_limits<std::uint32_t>::max())),
            static_cast<std::uint32_t>(wholeMember(
                document, "rows", 1,
                std::numeric_limits<std::uint32_t>::max())));

        if (schema == kSchemaVersion)
        {
            decodeLevels(document, map);
        }
        else
        {
            migrateLegacyColumns(document, map);
            decodeLegacyBridges(document, map);
            decodeLegacyWaters(document, map);
        }

        decodeEntities(document, map, schema);

        return map;
    }

    MapDocument mapDocumentFromJson(const nlohmann::json &document)
    {
        auto parsed = tileMapFromJson(document);
        MapDocument loaded{.map = std::move(parsed)}; // GCOVR_EXCL_LINE

        const auto schema = wholeMember(
            document, "schema", 0,
            std::numeric_limits<std::int64_t>::max());

        if (schema >= 2 && !document.contains("free"))
        {
            throw TileMapError(
                "the version 2 or later map lacks the free "
                "section");
        }

        loaded.free = decodeFree(document, loaded.map);

        return loaded;
    }

}
