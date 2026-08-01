#include "antwika/game/SaveGame.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/Walker.hpp"
#include "SaveMigration.hpp"

namespace antwika::game
{

    namespace
    {
        // Symbolic rather than the enumerator's number.
        // antwika::input writes key names rather than scancodes.
        // A name survives the enumeration being reordered.
        // Being hand-editable is most of why this format is JSON.
        // Indexed by directionIndex(), so the order is the enum's.
        constexpr std::array<std::string_view, kDirectionCount>
            kDirectionNames{"north", "east", "south", "west"};

        std::string_view nameOf(Direction direction)
        {
            return kDirectionNames[directionIndex(direction)];
        }

        Direction directionFromName(const std::string &name)
        {
            for (std::size_t index = 0; index < kDirectionNames.size();
                 ++index)
            {
                if (kDirectionNames[index] == name)
                {
                    return static_cast<Direction>(index);
                }
            }

            throw SaveFormatError(
                "antwika::game: a save names a direction that is not one "
                "of the four: " + name);
        }

        nlohmann::json coordinateShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = std::numeric_limits<std::int32_t>::min();
            shape["maximum"] = std::numeric_limits<std::int32_t>::max();
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json cellShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"x", "y"}; // GCOVR_EXCL_LINE
            shape["properties"]["x"] = coordinateShape();
            shape["properties"]["y"] = coordinateShape();
            return shape;
        }

        // An index into the other array, or absent for "nobody".
        // A negative one is refused by the schema rather than by hand.
        nlohmann::json linkShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json signedCountShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            shape["maximum"] = std::numeric_limits<std::int32_t>::max();
            return shape;
        } // GCOVR_EXCL_LINE

        // Every other integer here is capped at what its C++ type holds.
        // This one is capped tighter still, and deliberately.
        // WalkerSystem writes kTicksPerStep - 1 and counts it to zero.
        // So a phase at or above it is no state a run ever reaches.
        // SceneSnapshot works out kTicksPerStep - 1 - this, which underflows.
        // The decode is get<std::uint8_t>(), and nlohmann narrows quietly.
        // So a cap at what an int32 holds let 256 read back as zero.
        // Refusing is refusing a corrupt file, not merely a narrowed one.
        nlohmann::json stepPhaseShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            shape["maximum"] = kTicksPerStep - 1;
            return shape;
        } // GCOVR_EXCL_LINE

        // "facing" is a string here rather than a schema enum.
        // An unknown name is refused by directionFromName() instead.
        // That way the message holds the name it did not know.
        nlohmann::json walkerShape()
        {
            nlohmann::json shape = cellShape();
            // GCOVR_EXCL_START
            shape["required"] = {
                "x",
                "y",
                "facing",
                "kind",
                "carried",
                "stepsUntilHome",
                "ticksUntilStep"};
            // GCOVR_EXCL_STOP
            shape["properties"]["facing"]["type"] = "string";
            shape["properties"]["kind"]["type"] = "string";
            shape["properties"]["carried"] = signedCountShape();
            shape["properties"]["stepsUntilHome"] = signedCountShape();
            shape["properties"]["ticksUntilStep"] = stepPhaseShape();
            shape["properties"]["home"] = linkShape();
            return shape;
        }

        nlohmann::json buildingShape()
        {
            nlohmann::json shape = cellShape();
            // GCOVR_EXCL_START
            shape["required"] = {
                "x",
                "y",
                "kind",
                "stock",
                "risk",
                "ticksUntilSpawn",
                "ticksUntilDrain",
                "ticksUntilRisk"};
            // GCOVR_EXCL_STOP
            shape["properties"]["kind"]["type"] = "string";
            shape["properties"]["stock"]["type"] = "array";
            shape["properties"]["stock"]["items"] = signedCountShape();
            shape["properties"]["stock"]["minItems"] = kResourceCount;
            shape["properties"]["stock"]["maxItems"] = kResourceCount;
            shape["properties"]["risk"] = signedCountShape();
            shape["properties"]["ticksUntilSpawn"] = signedCountShape();
            shape["properties"]["ticksUntilDrain"] = signedCountShape();
            shape["properties"]["ticksUntilRisk"] = signedCountShape();
            shape["properties"]["walker"] = linkShape();
            return shape;
        }

        nlohmann::json countShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json stateShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"ticksProcessed", "score"}; // GCOVR_EXCL_LINE
            shape["properties"]["ticksProcessed"] = countShape();
            shape["properties"]["score"] = countShape();
            return shape;
        }

        nlohmann::json extentShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = {"width", "height"}; // GCOVR_EXCL_LINE
            shape["properties"]["width"] = coordinateShape();
            shape["properties"]["height"] = coordinateShape();
            return shape;
        }

        // Camera clamps a level it cannot honour, and keeps doing so.
        // That is right for zoomIn(), for zoomOut() and for a default.
        // It is wrong for a file, which is why the refusal lives here.
        // A level past kZoomHalfWidths is a camera no session ever had.
        // Loading it at the closest is a session somebody never played.
        // Which is the rule requireConsistentLinks() states below.
        nlohmann::json zoomLevelShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            shape["maximum"] = kZoomHalfWidths.size() - 1;
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json cameraShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            // GCOVR_EXCL_START
            shape["required"] = {"panX", "panY", "zoomLevel"};
            // GCOVR_EXCL_STOP
            shape["properties"]["panX"] = coordinateShape();
            shape["properties"]["panY"] = coordinateShape();
            shape["properties"]["zoomLevel"] = zoomLevelShape();
            return shape;
        }

        nlohmann::json arrayOf(nlohmann::json items)
        {
            nlohmann::json shape;
            shape["type"] = "array";
            shape["items"] = std::move(items);
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json saveSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika game save document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // The version member is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            // GCOVR_EXCL_START
            schema["required"] = {
                "magic",
                "state",
                "extent",
                "camera",
                "paths",
                "walkers",
                "buildings",
                "seed"};
            // GCOVR_EXCL_STOP
            schema["properties"]["magic"]["const"] =
                std::string(kSaveMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kSaveFormatVersion;
            schema["properties"]["state"] = stateShape();
            schema["properties"]["extent"] = extentShape();
            schema["properties"]["camera"] = cameraShape();
            schema["properties"]["paths"] = arrayOf(cellShape());
            schema["properties"]["walkers"] = arrayOf(walkerShape());
            schema["properties"]["buildings"] = arrayOf(buildingShape());
            schema["properties"]["seed"] = countShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &saveValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                saveSchema()); // GCOVR_EXCL_LINE
            return validator;
        }


        // Symbolic for the same reason a direction is.
        // A name survives the enumeration being reordered.
        constexpr std::array<std::string_view, kWalkerKindCount>
            kWalkerKindNames{"food", "water", "fireman", "architect"};

        std::string_view walkerKindName(WalkerKind kind)
        {
            return kWalkerKindNames[walkerKindIndex(kind)];
        }

        WalkerKind walkerKindFromJson(const std::string &name)
        {
            for (std::size_t index = 0; index < kWalkerKindNames.size();
                 ++index)
            {
                if (kWalkerKindNames[index] == name)
                {
                    return static_cast<WalkerKind>(index);
                }
            }

            throw SaveFormatError(
                "antwika::game: a save names a walker kind that is not "
                "one of the four: " + name);
        }

        BuildingKind buildingKindFromJson(const std::string &name)
        {
            const auto kind = buildingKindFromName(name);

            if (!kind.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a building kind this "
                    "build does not have: " + name);
            }

            return *kind;
        }

        std::array<std::int32_t, kResourceCount> stockFromJson(
            const nlohmann::json &j)
        {
            std::array<std::int32_t, kResourceCount> stock{};

            for (std::size_t index = 0; index < kResourceCount; ++index)
            {
                stock[index] = j.at(index).get<std::int32_t>();
            }

            return stock;
        }

        // Absent means nobody, which is an ordinary state.
        // Rather than a field somebody forgot to write.
        std::optional<std::size_t> linkFromJson(
            const nlohmann::json &j, const char *key)
        {
            if (!j.contains(key))
            {
                return std::nullopt;
            }

            return j.at(key).get<std::size_t>();
        }

        // An index past the end of the array it points into is corrupt.
        // So is a pair that disagree about each other.
        // This project refuses one rather than repairing it.
        // A repaired save is a session somebody never had.
        void requireConsistentLinks(const SaveGame &save)
        {
            for (std::size_t index = 0; index < save.walkers.size(); ++index)
            {
                const auto &home = save.walkers[index].home;

                if (!home.has_value())
                {
                    continue;
                }

                if (*home >= save.buildings.size())
                {
                    throw SaveFormatError(
                        "antwika::game: a save names a walker whose home "
                        "is not a building in it");
                }

                const auto &back = save.buildings[*home].walker;

                if (!back.has_value() || *back != index)
                {
                    throw SaveFormatError(
                        "antwika::game: a save names a walker and a "
                        "building that disagree about each other");
                }
            }

            for (const auto &building : save.buildings)
            {
                if (building.walker.has_value()
                    && *building.walker >= save.walkers.size())
                {
                    throw SaveFormatError(
                        "antwika::game: a save names a building whose "
                        "walker is not a walker in it");
                }
            }
        }

        Cell cellFromJson(const nlohmann::json &j)
        {
            return Cell{
                .x = j.at("x").get<std::int32_t>(),
                .y = j.at("y").get<std::int32_t>(),
            };
        }

        nlohmann::json cellToJson(Cell cell)
        {
            nlohmann::json encoded;
            encoded["x"] = cell.x;
            encoded["y"] = cell.y;
            return encoded;
        } // GCOVR_EXCL_LINE
    } // namespace

    nlohmann::json saveGameToJson(const SaveGame &save)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kSaveMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kSaveFormatVersion;
        encoded["state"]["ticksProcessed"] = save.state.ticksProcessed;
        encoded["state"]["score"] = save.state.score;
        encoded["extent"]["width"] = save.extent.width;
        encoded["extent"]["height"] = save.extent.height;
        encoded["camera"]["panX"] = save.camera.pan().x;
        encoded["camera"]["panY"] = save.camera.pan().y;
        encoded["camera"]["zoomLevel"] = save.camera.zoomLevel();
        encoded["paths"] = nlohmann::json::array();
        for (const auto cell : save.paths)
        {
            encoded["paths"].push_back(cellToJson(cell));
        }

        encoded["walkers"] = nlohmann::json::array();
        for (const auto &walker : save.walkers)
        {
            auto entry = cellToJson(walker.at);
            entry["facing"] = std::string(nameOf(walker.facing));
            entry["kind"] = std::string(walkerKindName(walker.kind));
            entry["carried"] = walker.carried;
            entry["stepsUntilHome"] = walker.stepsUntilHome;
            entry["ticksUntilStep"] = walker.ticksUntilStep;

            if (walker.home.has_value())
            {
                entry["home"] = *walker.home;
            }

            encoded["walkers"].push_back(std::move(entry));
        }

        encoded["buildings"] = nlohmann::json::array();
        for (const auto &building : save.buildings)
        {
            auto entry = cellToJson(building.at);
            entry["kind"] = std::string(buildingKindName(building.kind));
            entry["stock"] = building.stock;
            entry["risk"] = building.risk;
            entry["ticksUntilSpawn"] = building.ticksUntilSpawn;
            entry["ticksUntilDrain"] = building.ticksUntilDrain;
            entry["ticksUntilRisk"] = building.ticksUntilRisk;

            if (building.walker.has_value())
            {
                entry["walker"] = *building.walker;
            }

            encoded["buildings"].push_back(std::move(entry));
        }

        encoded["seed"] = save.seed;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // ReplayJson.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    SaveGame saveGameFromJson(const nlohmann::json &j)
    {
        auto document = j;
        detail::migrateSaveDocument(document);

        try
        {
            saveValidator().validate(document);
        }
        catch (const std::exception &error) // GCOVR_EXCL_LINE
        {
            throw SaveFormatError(
                std::string("antwika::game: save JSON failed schema "
                            "validation: ")
                + error.what());
        }

        SaveGame save;
        save.state.ticksProcessed =
            document.at("state").at("ticksProcessed").get<std::uint64_t>();
        save.state.score =
            document.at("state").at("score").get<std::uint64_t>();
        save.extent = GridExtent{
            .width = document.at("extent").at("width").get<std::int32_t>(),
            .height =
                document.at("extent").at("height").get<std::int32_t>(),
        };
        const Point pan{
            .x = document.at("camera").at("panX").get<std::int32_t>(),
            .y = document.at("camera").at("panY").get<std::int32_t>(),
        };
        const auto zoom =
            document.at("camera").at("zoomLevel").get<std::size_t>();
        save.camera = Camera(pan, zoom);

        for (const auto &cell : document.at("paths"))
        {
            save.paths.push_back(cellFromJson(cell));
        }

        for (const auto &walker : document.at("walkers"))
        {
            save.walkers.push_back(SavedWalker{
                .at = cellFromJson(walker),
                .facing = directionFromName(
                    walker.at("facing").get<std::string>()),
                .kind = walkerKindFromJson(
                    walker.at("kind").get<std::string>()),
                .carried = walker.at("carried").get<std::int32_t>(),
                .stepsUntilHome =
                    walker.at("stepsUntilHome").get<std::int32_t>(),
                .ticksUntilStep =
                    walker.at("ticksUntilStep").get<std::uint8_t>(),
                .home = linkFromJson(walker, "home"),
            });
        }

        for (const auto &building : document.at("buildings"))
        {
            save.buildings.push_back(SavedBuilding{
                .at = cellFromJson(building),
                .kind = buildingKindFromJson(
                    building.at("kind").get<std::string>()),
                .stock = stockFromJson(building.at("stock")),
                .risk = building.at("risk").get<std::int32_t>(),
                .ticksUntilSpawn =
                    building.at("ticksUntilSpawn").get<std::int32_t>(),
                .ticksUntilDrain =
                    building.at("ticksUntilDrain").get<std::int32_t>(),
                .ticksUntilRisk =
                    building.at("ticksUntilRisk").get<std::int32_t>(),
                .walker = linkFromJson(building, "walker"),
            });
        }

        save.seed = document.at("seed").get<std::uint64_t>();

        requireConsistentLinks(save);

        return save;
    } // GCOVR_EXCL_LINE

    SaveGame saveGameOf(
        const antwika::ecs::World &world,
        const PathIndex &paths,
        const Camera &camera,
        const GameState &state,
        GridExtent extent,
        std::uint64_t seed)
    {
        SaveGame save;
        save.state = state;
        save.extent = extent;
        save.camera = camera;
        save.paths.assign(paths.cells().begin(), paths.cells().end());
        save.seed = seed;

        // Walked once each, keeping where every entity landed.
        // So the second pass turns a handle into a record's index.
        std::map<antwika::ecs::Entity, std::size_t> walkerAt;
        std::map<antwika::ecs::Entity, std::size_t> buildingAt;

        for (const auto entity : world.view<Walker, Cell>())
        {
            walkerAt.emplace(entity, save.walkers.size());
            const auto walker = world.get<Walker>(entity);

            save.walkers.push_back(SavedWalker{
                .at = world.get<Cell>(entity),
                .facing = walker.facing,
                .kind = walker.kind,
                .carried = walker.carried,
                .stepsUntilHome = walker.stepsUntilHome,
                .ticksUntilStep = walker.ticksUntilStep,
                .home = std::nullopt});
        }

        for (const auto entity : world.view<Building, Cell>())
        {
            buildingAt.emplace(entity, save.buildings.size());
            const auto building = world.get<Building>(entity);

            save.buildings.push_back(SavedBuilding{
                .at = world.get<Cell>(entity),
                .kind = building.kind,
                .stock = building.stock,
                .risk = building.risk,
                .ticksUntilSpawn = building.ticksUntilSpawn,
                .ticksUntilDrain = building.ticksUntilDrain,
                .ticksUntilRisk = building.ticksUntilRisk,
                .walker = std::nullopt});
        }

        // The link, written only where both ends were recorded.
        // A building whose walker is not in the file has nobody out.
        // Which is exactly what it will be on the way back.
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto walker = world.get<Building>(entity).walker;
            const auto found = walkerAt.find(walker);

            if (found == walkerAt.end())
            {
                continue;
            }

            save.buildings[buildingAt.at(entity)].walker = found->second;
            save.walkers[found->second].home = buildingAt.at(entity);
        }

        return save;
    } // GCOVR_EXCL_LINE

    PathIndex pathIndexOf(const SaveGame &save)
    {
        PathIndex index;
        for (const auto cell : save.paths)
        {
            (void)index.insert(cell);
        }

        return index;
    } // GCOVR_EXCL_LINE

} // namespace antwika::game
