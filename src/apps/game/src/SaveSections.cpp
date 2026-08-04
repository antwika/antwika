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
        // The shapes every persisted format shares are replay's.
        // The ones only a save has are the ones here.
        using replay::countShape;

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

        // Symbolic for the same reason a direction is.
        // A name survives the enumeration being reordered.
        constexpr std::array<std::string_view, kWalkerKindCount>
            kWalkerKindNames{
                "water_carrier",
                "doctor",
                "fireman",
                "engineer",
                "cart_pusher",
                "market_buyer",
                "market_seller",
                "migrant",
                "labourer"};

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
                "antwika::game: a save names a walker kind this build "
                "does not have: " + name);
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

        // Every other integer here is capped at what its C++ type holds.
        // This one is capped tighter still, and deliberately.
        // WalkerSystem writes kTicksPerStep - 1 and counts it to zero.
        // So a phase at or above it is no state a run ever reaches.
        // SceneSnapshot works out kTicksPerStep - 1 - this.
        // Which underflows.
        // The decode is get<std::uint8_t>(), which narrows quietly.
        // So a cap at what an int32 holds let 256 read back as zero.
        // Refusing one is refusing a corrupt file.
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
            // The excluded line is the local vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE
    } // namespace

    nlohmann::json cellShape()
    {
        nlohmann::json shape = replay::objectShape({"x", "y"});
        shape["properties"]["x"] = replay::coordinateShape();
        shape["properties"]["y"] = replay::coordinateShape();
        return shape;
    }

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

    // "facing" is a string here rather than a schema enum.
    // An unknown name is refused by directionFromName() instead.
    // That way the message holds the name it did not know.
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

        // An index into the buildings array, or absent for "nobody".
        // A negative one is refused by the schema, not by hand.
        shape["properties"]["home"] = countShape();

        // One line per section, and each in a file of its own.
        describeErrand(shape);
        describeJourney(shape);
        describeFireCall(shape);
        return shape;
    }

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

        // Optional, and absent means no collapse risk at all.
        // Which is what a file written before the split says.
        // Its combined "risk" reads as the fire risk alone.
        shape["properties"]["collapseRisk"] = signedCountShape();

        // Optional on the same terms, for the disease risk.
        // Absent is what a file written before it existed says.
        shape["properties"]["diseaseRisk"] = signedCountShape();
        shape["properties"]["ticksUntilSpawn"] = signedCountShape();
        shape["properties"]["ticksUntilDrain"] = signedCountShape();
        shape["properties"]["ticksUntilRisk"] = signedCountShape();

        // Indices into the walkers array, capped at what one holds.
        // A file naming a third names a slot this build has not got.
        // Which is a file to refuse rather than one to read two of.
        shape["properties"]["walkers"]["type"] = "array";
        shape["properties"]["walkers"]["items"] = countShape();
        shape["properties"]["walkers"]["maxItems"] = kMaxWalkersOut;

        // One line per section, and each in a file of its own.
        describeProduction(shape);
        return shape;
    }

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

            // Written only when there is any, as the walkers are.
            // Absent and zero read the same.
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

            // Written only when there is somebody out.
            // An empty array and an absent member read the same.
            // So the smaller file is the one worth writing.
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
            // The excluded line carries the same landing pad.
            // As the push_back in saveGameOf() does, at length.
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

} // namespace antwika::game
