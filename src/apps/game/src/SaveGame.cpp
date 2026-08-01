#include "antwika/game/SaveGame.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/Walker.hpp"
#include "SaveSections.hpp"

namespace antwika::game
{

    namespace
    {
        using replay::coordinateShape;
        using replay::countShape;

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
        // Which is the rule requireConsistentLinks() states too.
        nlohmann::json zoomLevelShape()
        {
            return replay::boundedCountShape(
                static_cast<std::int64_t>(kZoomHalfWidths.size() - 1));
        }

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

        // **The spine, and the one statement of the shape.**
        // A section adds its own members to the shapes here.
        // See SaveSections.hpp.
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

        walkersToJson(save, encoded);
        buildingsToJson(save, encoded);

        encoded["seed"] = save.seed;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // ReplayJson.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    SaveGame saveGameFromJson(const nlohmann::json &j)
    {
        const auto document =
            replay::readVersionedDocument<SaveFormatError>(
                j,
                standardSaveMigrations(),
                saveValidator(),
                "antwika::game: save JSON failed schema validation: ");

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

        walkersFromJson(document, save);
        buildingsFromJson(document, save);

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
                .walkers = {}});
        }

        // The links, written only where both ends were recorded.
        // A building whose walker is not in the file has that slot free.
        // Which is exactly what it will be on the way back.
        //
        // Written in slot order and closed up.
        // A slot number is not a role -- see Building::walkers.
        // So a walker held in the second slot alone comes back first.
        for (const auto entity : world.view<Building, Cell>())
        {
            const auto held = world.get<Building>(entity).walkers;

            for (const auto walker : held)
            {
                const auto found = walkerAt.find(walker);

                if (found == walkerAt.end())
                {
                    continue;
                }

                save.buildings[buildingAt.at(entity)].walkers.push_back(
                    found->second);
                save.walkers[found->second].home = buildingAt.at(entity);
            }
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
