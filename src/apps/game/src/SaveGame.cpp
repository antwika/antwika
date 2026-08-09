#include "antwika/game/SaveGame.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/Coverage.hpp"
#include "antwika/game/Errand.hpp"
#include "antwika/game/FireCall.hpp"
#include "antwika/game/Ruin.hpp"
#include "antwika/game/Journey.hpp"
#include "antwika/game/Household.hpp"
#include "antwika/game/Production.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Staff.hpp"
#include "antwika/game/Employment.hpp"
#include "SaveSections.hpp"

namespace antwika::game
{

    namespace
    {
        using replay::coordinateShape;
        using replay::countShape;

        nlohmann::json moneyShape()
        {
            return antwika::config::wholeShape(
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::max());
        }

        nlohmann::json stateShape()
        {
            nlohmann::json shape =
                replay::objectShape({"ticksProcessed", "score"});
            shape["properties"]["ticksProcessed"] = countShape();
            shape["properties"]["score"] = countShape();

            shape["properties"]["money"] = moneyShape();
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json extentShape()
        {
            nlohmann::json shape =
                replay::objectShape({"width", "height"});
            shape["properties"]["width"] = coordinateShape();
            shape["properties"]["height"] = coordinateShape();
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json zoomLevelShape()
        {
            return replay::boundedCountShape(
                static_cast<std::int64_t>(kZoomHalfWidths.size() - 1));
        }

        nlohmann::json cameraShape()
        {
            nlohmann::json shape =
                replay::objectShape({"panX", "panY", "zoomLevel"});
            shape["properties"]["panX"] = coordinateShape();
            shape["properties"]["panY"] = coordinateShape();
            shape["properties"]["zoomLevel"] = zoomLevelShape();
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json arrayOf(nlohmann::json items)
        {
            nlohmann::json shape;
            shape["type"] = "array";
            shape["items"] = std::move(items);
            return shape;
        } // GCOVR_EXCL_LINE

        nlohmann::json saveSchema()
        {
            nlohmann::json schema = replay::documentShape(
                "antwika game save document",
                {"magic",
                 "state",
                 "extent",
                 "camera",
                 "paths",
                 "walkers",
                 "buildings",
                 "seed"});
            schema["properties"]["magic"]["const"] =
                std::string(kSaveMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kSaveFormatVersion;
            schema["properties"]["state"] = stateShape();
            schema["properties"]["extent"] = extentShape();
            schema["properties"]["camera"] = cameraShape();
            schema["properties"]["paths"] = arrayOf(cellShape());

            auto walker = walkerShape();
            auto building = buildingShape();
            SaveShapes shapes{schema, walker, building};

            for (const auto &section : saveSections())
            {
                if (section.describe != nullptr)
                {
                    section.describe(shapes);
                }
            }

            schema["properties"]["walkers"] = arrayOf(std::move(walker));
            schema["properties"]["buildings"] =
                arrayOf(std::move(building));
            schema["properties"]["seed"] = countShape();
            return schema;
        }
    }

    nlohmann::json saveGameToJson(const SaveGame &save)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kSaveMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kSaveFormatVersion;
        encoded["state"]["ticksProcessed"] = save.state.ticksProcessed;
        encoded["state"]["score"] = save.state.score;
        encoded["state"]["money"] = save.state.money;
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

        for (const auto &section : saveSections())
        {
            if (section.encode != nullptr)
            {
                section.encode(save, encoded);
            }
        }

        encoded["seed"] = save.seed;
        return encoded;

    } // GCOVR_EXCL_LINE

    SaveGame saveGameFromJson(const nlohmann::json &j)
    {
        const auto document =
            antwika::config::migratedAs<SaveFormatError>(
                j,
                standardSaveMigrations(),
                replay::validatorFor<saveSchema>(),
                "antwika::game: save JSON failed schema validation: ");

        SaveGame save;
        save.state.ticksProcessed =
            document.at("state").at("ticksProcessed").get<std::uint64_t>();
        save.state.score =
            document.at("state").at("score").get<std::uint64_t>();

        if (document.at("state").contains("money"))
        {
            save.state.money =
                document.at("state").at("money").get<std::int64_t>();
        }
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

        for (const auto &section : saveSections())
        {
            if (section.decode != nullptr)
            {
                section.decode(document, save);
            }
        }

        save.seed = document.at("seed").get<std::uint64_t>();

        for (const auto &section : saveSections())
        {
            if (section.check != nullptr)
            {
                section.check(save);
            }
        }

        return save;
    } // GCOVR_EXCL_LINE

    SaveGame saveGameFrom(
        const CityGrid &grid,
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

        save.walkers.reserve(grid.walkers.size());
        save.buildings.reserve(grid.buildings.size());
        save.ruins.reserve(grid.ruins.size());

        for (const auto &stored : grid.walkers)
        {
            std::optional<SavedErrand> errand;

            if (stored.errand.has_value())
            {
                errand = SavedErrand{
                    .destination = stored.destination,
                    .carrying = stored.errand->carrying,
                    .leg = stored.errand->leg};
            }

            std::optional<SavedJourney> journey;

            if (stored.journey.has_value())
            {
                journey = SavedJourney{
                    .towards = stored.journey->towards,
                    .house = stored.joining};
            }

            save.walkers.push_back(SavedWalker{
                .at = stored.at,
                .facing = stored.walker.facing,
                .kind = stored.walker.kind,
                .carried = stored.walker.carried,
                .stepsUntilHome = stored.walker.stepsUntilHome,
                .ticksUntilStep = stored.walker.ticksUntilStep,
                .home = stored.home,
                .errand = errand,
                .journey = journey,
                .fireCall = stored.attending});
        }

        for (const auto &stored : grid.buildings)
        {
            std::vector<std::size_t> out;

            for (const auto slot : stored.walkers)
            {
                if (slot.has_value())
                {
                    out.push_back(*slot);
                }
            }

            std::optional<std::int32_t> countdown;

            if (stored.production.has_value())
            {
                countdown = stored.production->ticksUntilOutput;
            }

            save.buildings.push_back(SavedBuilding{ // GCOVR_EXCL_LINE
                .at = stored.at,
                .kind = stored.building.kind,
                .stock = stored.building.stock,
                .risk = stored.building.fireRisk,
                .collapseRisk = stored.building.collapseRisk,
                .diseaseRisk = stored.building.diseaseRisk,
                .ticksUntilSpawn = stored.building.ticksUntilSpawn,
                .ticksUntilDrain = stored.building.ticksUntilDrain,
                .ticksUntilRisk = stored.building.ticksUntilRisk,
                .selling = stored.building.selling,
                .walkers = std::move(out),
                .coverage = stored.coverage.ticksLeft,
                .ticksUntilOutput = countdown,
                .household = stored.household,
                .staff = stored.staff,
                .employment = stored.employment});
        }

        for (const auto &stored : grid.ruins)
        {
            save.ruins.push_back(SavedRuin{
                .at = stored.at,
                .kind = stored.ruin.kind,
                .state = stored.ruin.state,
                .ticksUntilOut = stored.ruin.ticksUntilOut});
        }

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
        return saveGameFrom(
            cityGridOf(world), paths, camera, state, extent, seed);
    }

    PathIndex pathIndexOf(const SaveGame &save)
    {
        PathIndex index;
        for (const auto cell : save.paths)
        {
            (void)index.insert(cell);
        }

        return index;
    } // GCOVR_EXCL_LINE

}
