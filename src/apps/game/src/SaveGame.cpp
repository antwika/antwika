#include "antwika/game/SaveGame.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>

#include <nlohmann/json-schema.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/SaveFormatError.hpp"
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
        }

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

        // "facing" is a string here rather than a schema enum.
        // An unknown name is refused by directionFromName() instead.
        // That way the message holds the name it did not know.
        nlohmann::json walkerShape()
        {
            nlohmann::json shape = cellShape();
            shape["required"] = {"x", "y", "facing"}; // GCOVR_EXCL_LINE
            shape["properties"]["facing"]["type"] = "string";
            return shape;
        }

        nlohmann::json countShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            return shape;
        }

        nlohmann::json stateShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = // GCOVR_EXCL_LINE
                {"ticksProcessed", "score"};
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

        nlohmann::json cameraShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            shape["required"] = // GCOVR_EXCL_LINE
                {"panX", "panY", "zoomLevel"};
            shape["properties"]["panX"] = coordinateShape();
            shape["properties"]["panY"] = coordinateShape();
            shape["properties"]["zoomLevel"] = countShape();
            return shape;
        }

        nlohmann::json arrayOf(nlohmann::json items)
        {
            nlohmann::json shape;
            shape["type"] = "array";
            shape["items"] = std::move(items);
            return shape;
        }

        nlohmann::json saveSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika game save document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // schemaVersion is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            schema["required"] = // GCOVR_EXCL_LINE
                {"magic",
                 "state",
                 "extent",
                 "camera",
                 "paths",
                 "walkers",
                 "seed"};
            schema["properties"]["magic"]["const"] =
                std::string(kSaveMagic);
            schema["properties"]["schemaVersion"]["const"] =
                kSaveFormatVersion;
            schema["properties"]["state"] = stateShape();
            schema["properties"]["extent"] = extentShape();
            schema["properties"]["camera"] = cameraShape();
            schema["properties"]["paths"] = arrayOf(cellShape());
            schema["properties"]["walkers"] = arrayOf(walkerShape());
            schema["properties"]["seed"] = countShape();
            return schema;
        }

        const nlohmann::json_schema::json_validator &saveValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                saveSchema()); // GCOVR_EXCL_LINE
            return validator;
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
        encoded["schemaVersion"] = kSaveFormatVersion;
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
            encoded["walkers"].push_back(std::move(entry));
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
        detail::migrateSaveDocument(document, detail::saveVersionOf(j));

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
        save.camera = Camera(
            Point{
                .x = document.at("camera").at("panX").get<std::int32_t>(),
                .y = document.at("camera").at("panY").get<std::int32_t>(),
            },
            document.at("camera").at("zoomLevel").get<std::size_t>());

        for (const auto &cell : document.at("paths"))
        {
            save.paths.push_back(cellFromJson(cell));
        }

        for (const auto &walker : document.at("walkers"))
        {
            save.walkers.push_back(WalkerView{
                .at = cellFromJson(walker),
                .facing = directionFromName(
                    walker.at("facing").get<std::string>()),
            });
        }

        save.seed = document.at("seed").get<std::uint64_t>();
        return save;
    } // GCOVR_EXCL_LINE

    SaveGame saveGameOf(
        const GameSummary &summary, GridExtent extent, std::uint64_t seed)
    {
        SaveGame save;
        save.state = summary.state;
        save.extent = extent;
        save.camera = summary.camera;
        save.paths = summary.paths;
        save.walkers = summary.walkers;
        save.seed = seed;
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
