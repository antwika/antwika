#include "SaveSections.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/enums/FromName.hpp>
#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Building.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    namespace
    {
        using replay::countShape;

        constexpr antwika::enums::NameTable<Direction>
            kDirectionNames{{"north", "east", "south", "west"}};

        std::string_view nameOf(Direction direction)
        {
            return kDirectionNames.name(direction);
        }

        Direction directionFromName(const std::string &name)
        {
            return antwika::enums::fromName<SaveFormatError>(
                kDirectionNames,
                name,
                "antwika::game: a save names a direction that is not "
                "one of the four: ");
        }

        constexpr antwika::enums::NameTable<WalkerKind>
            kWalkerKindNames{{
                "water_carrier",
                "doctor",
                "fireman",
                "engineer",
                "cart_pusher",
                "market_buyer",
                "market_seller",
                "migrant",
                "labourer"}};

        std::string_view walkerKindName(WalkerKind kind)
        {
            return kWalkerKindNames.name(kind);
        }

        WalkerKind walkerKindFromJson(const std::string &name)
        {
            return antwika::enums::fromName<SaveFormatError>(
                kWalkerKindNames,
                name,
                "antwika::game: a save names a walker kind this build "
                "does not have: ");
        }

        BuildingKind buildingKindFromJson(const std::string &name)
        {
            return antwika::enums::orThrow<SaveFormatError>(
                buildingKindFromName(name),
                "antwika::game: a save names a building kind this "
                "build does not have: ",
                name);
        }

        Resource resourceFromJson(const std::string &name)
        {
            return antwika::enums::orThrow<SaveFormatError>(
                resourceFromName(name),
                "antwika::game: a save names a resource this build "
                "does not have: ",
                name);
        }

        Resource sellingFromJson(const nlohmann::json &building)
        {
            if (!building.contains("selling"))
            {
                return Resource::Food;
            }

            return resourceFromJson(
                building.at("selling").get<std::string>());
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

        nlohmann::json stepPhaseShape()
        {
            return replay::boundedCountShape(kTicksPerStep - 1);
        }

        std::vector<std::size_t> linkListFromJson(const nlohmann::json &j)
        {
            std::vector<std::size_t> indices;

            if (!j.contains("walkers"))
            {
                return indices;
            }

            for (const auto &index : j.at("walkers"))
            {
                indices.push_back(index.get<std::size_t>());
            }

            return indices;
        } // GCOVR_EXCL_LINE
    }

    nlohmann::json cellShape()
    {
        nlohmann::json shape = replay::objectShape({"x", "y"});
        shape["properties"]["x"] = replay::coordinateShape();
        shape["properties"]["y"] = replay::coordinateShape();
        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json signedCountShape()
    {
        return replay::boundedCountShape(
            std::numeric_limits<std::int32_t>::max());
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

    std::optional<std::size_t> linkFromJson(
        const nlohmann::json &j, const char *key)
    {
        if (!j.contains(key))
        {
            return std::nullopt;
        }

        return j.at(key).get<std::size_t>();
    }

    nlohmann::json walkerShape()
    {
        nlohmann::json shape = cellShape();
        shape["required"] = replay::requiredShape(
            {"x",
             "y",
             "facing",
             "kind",
             "carried",
             "stepsUntilHome",
             "ticksUntilStep"});
        shape["properties"]["facing"] = replay::wordShape();
        shape["properties"]["kind"] = replay::wordShape();
        shape["properties"]["carried"] = signedCountShape();
        shape["properties"]["stepsUntilHome"] = signedCountShape();
        shape["properties"]["ticksUntilStep"] = stepPhaseShape();

        shape["properties"]["home"] = countShape();

        return shape;
    } // GCOVR_EXCL_LINE

    nlohmann::json buildingShape()
    {
        nlohmann::json shape = cellShape();
        shape["required"] = replay::requiredShape(
            {"x",
             "y",
             "kind",
             "stock",
             "risk",
             "ticksUntilSpawn",
             "ticksUntilDrain",
             "ticksUntilRisk"});
        shape["properties"]["kind"] = replay::wordShape();
        shape["properties"]["stock"]["type"] = "array";
        shape["properties"]["stock"]["items"] = signedCountShape();
        shape["properties"]["stock"]["minItems"] = kResourceCount;
        shape["properties"]["stock"]["maxItems"] = kResourceCount;
        shape["properties"]["risk"] = signedCountShape();

        shape["properties"]["collapseRisk"] = signedCountShape();

        shape["properties"]["diseaseRisk"] = signedCountShape();
        shape["properties"]["ticksUntilSpawn"] = signedCountShape();
        shape["properties"]["ticksUntilDrain"] = signedCountShape();
        shape["properties"]["ticksUntilRisk"] = signedCountShape();

        shape["properties"]["selling"] = replay::wordShape();

        shape["properties"]["walkers"]["type"] = "array";
        shape["properties"]["walkers"]["items"] = countShape();
        shape["properties"]["walkers"]["maxItems"] = kMaxWalkersOut;

        return shape;
    } // GCOVR_EXCL_LINE

    void walkersToJson(const SaveGame &save, nlohmann::json &document)
    {
        document["walkers"] = nlohmann::json::array();

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

            document["walkers"].push_back(std::move(entry));
        }
    }

    void buildingsToJson(const SaveGame &save, nlohmann::json &document)
    {
        document["buildings"] = nlohmann::json::array();

        for (const auto &building : save.buildings)
        {
            auto entry = cellToJson(building.at);
            entry["kind"] = std::string(buildingKindName(building.kind));
            entry["stock"] = building.stock;
            entry["risk"] = building.risk;

            if (building.collapseRisk != 0)
            {
                entry["collapseRisk"] = building.collapseRisk;
            }

            if (building.diseaseRisk != 0)
            {
                entry["diseaseRisk"] = building.diseaseRisk;
            }

            entry["ticksUntilSpawn"] = building.ticksUntilSpawn;
            entry["ticksUntilDrain"] = building.ticksUntilDrain;
            entry["ticksUntilRisk"] = building.ticksUntilRisk;

            if (building.selling != Resource::Food)
            {
                entry["selling"] =
                    std::string(resourceName(building.selling));
            }

            if (!building.walkers.empty())
            {
                entry["walkers"] = building.walkers;
            }

            document["buildings"].push_back(std::move(entry));
        }
    }

    void walkersFromJson(const nlohmann::json &document, SaveGame &save)
    {
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
    }

    void buildingsFromJson(const nlohmann::json &document, SaveGame &save)
    {
        for (const auto &building : document.at("buildings"))
        {
            save.buildings.push_back(SavedBuilding{ // GCOVR_EXCL_LINE
                .at = cellFromJson(building),
                .kind = buildingKindFromJson(
                    building.at("kind").get<std::string>()),
                .stock = stockFromJson(building.at("stock")),
                .risk = building.at("risk").get<std::int32_t>(),
                .collapseRisk = building.contains("collapseRisk")
                    ? building.at("collapseRisk").get<std::int32_t>()
                    : 0,
                .diseaseRisk = building.contains("diseaseRisk")
                    ? building.at("diseaseRisk").get<std::int32_t>()
                    : 0,
                .ticksUntilSpawn =
                    building.at("ticksUntilSpawn").get<std::int32_t>(),
                .ticksUntilDrain =
                    building.at("ticksUntilDrain").get<std::int32_t>(),
                .ticksUntilRisk =
                    building.at("ticksUntilRisk").get<std::int32_t>(),
                .selling = sellingFromJson(building),
                .walkers = linkListFromJson(building),
            });
        }
    }

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

            const auto &back = save.buildings[*home].walkers;

            if (std::ranges::find(back, index) == back.end())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a walker and a "
                    "building that disagree about each other");
            }
        }

        for (const auto &building : save.buildings)
        {
            for (const auto walker : building.walkers)
            {
                if (walker >= save.walkers.size())
                {
                    throw SaveFormatError(
                        "antwika::game: a save names a building whose "
                        "walker is not a walker in it");
                }
            }
        }
    }

    namespace
    {
        void describeWalkerErrand(SaveShapes &shapes)
        {
            describeErrand(shapes.walker);
        }

        void describeWalkerJourney(SaveShapes &shapes)
        {
            describeJourney(shapes.walker);
        }

        void describeWalkerFireCall(SaveShapes &shapes)
        {
            describeFireCall(shapes.walker);
        }

        void describeBuildingCoverage(SaveShapes &shapes)
        {
            describeCoverage(shapes.building);
        }

        void describeBuildingHousing(SaveShapes &shapes)
        {
            describeHousing(shapes.building);
        }

        void describeBuildingLabour(SaveShapes &shapes)
        {
            describeLabour(shapes.building);
        }

        void describeBuildingProduction(SaveShapes &shapes)
        {
            describeProduction(shapes.building);
        }

        void describeDocumentRuins(SaveShapes &shapes)
        {
            describeRuins(shapes.document);
        }

        constexpr std::array<SaveSection, 10> kSaveSections{{
            {.name = "walkers",
             .encode = walkersToJson,
             .decode = walkersFromJson,
             .check = requireConsistentLinks},

            {.name = "buildings",
             .encode = buildingsToJson,
             .decode = buildingsFromJson},

            {.name = "coverage",
             .describe = describeBuildingCoverage,
             .encode = coverageToJson,
             .decode = coverageFromJson},

            {.name = "housing",
             .describe = describeBuildingHousing,
             .encode = housingToJson,
             .decode = housingFromJson},

            {.name = "labour",
             .describe = describeBuildingLabour,
             .encode = labourToJson,
             .decode = labourFromJson,
             .check = requireConsistentStaffing},

            {.name = "production",
             .describe = describeBuildingProduction,
             .encode = productionToJson,
             .decode = productionFromJson},

            {.name = "errands",
             .describe = describeWalkerErrand,
             .check = requireConsistentErrands},

            {.name = "migrants",
             .describe = describeWalkerJourney,
             .encode = migrantsToJson,
             .decode = migrantsFromJson,
             .check = requireConsistentJourneys},

            {.name = "ruins",
             .describe = describeDocumentRuins,
             .encode = ruinsToJson,
             .decode = ruinsFromJson},

            {.name = "fire calls",
             .describe = describeWalkerFireCall,
             .check = requireConsistentFireCalls},
        }};
    }

    std::span<const SaveSection> saveSections()
    {
        return kSaveSections;
    }

}
