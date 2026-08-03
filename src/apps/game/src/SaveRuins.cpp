#include "SaveSections.hpp"

#include <cstddef>
#include <string>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Ruin.hpp"
#include "antwika/game/SaveFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        // Refused with the name in the message, as a direction is.
        // A schema enum would refuse without saying what it saw.
        [[nodiscard]] RuinState ruinStateFromJson(const std::string &name)
        {
            const auto state = ruinStateFromName(name);

            if (!state.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a ruin state this "
                    "build does not have: " + name);
            }

            return *state;
        }

        [[nodiscard]] BuildingKind ruinKindFromJson(
            const std::string &name)
        {
            const auto kind = buildingKindFromName(name);

            if (!kind.has_value())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a ruin whose kind "
                    "this build does not have: " + name);
            }

            return *kind;
        }
    } // namespace

    nlohmann::json ruinShape()
    {
        nlohmann::json shape = cellShape();
        // GCOVR_EXCL_START
        shape["required"] = {"x", "y", "kind", "state", "ticksUntilOut"};
        // GCOVR_EXCL_STOP
        shape["properties"]["kind"] = replay::wordShape();
        shape["properties"]["state"] = replay::wordShape();
        shape["properties"]["ticksUntilOut"] = signedCountShape();
        return shape;
    }

    void describeRuins(nlohmann::json &schema)
    {
        auto &ruins = schema["properties"]["ruins"];
        ruins["type"] = "array";
        ruins["items"] = ruinShape();
    }

    void describeFireCall(nlohmann::json &walker)
    {
        // An index into the ruins array, or absent for nobody's call.
        // A negative one is refused by the schema, not by hand.
        walker["properties"]["fireCall"] = replay::countShape();
    }

    void ruinsToJson(const SaveGame &save, nlohmann::json &document)
    {
        // Written only when something has burnt.
        // An empty array and an absent member read the same.
        // So the smaller file is the one worth writing.
        if (!save.ruins.empty())
        {
            document["ruins"] = nlohmann::json::array();

            for (const auto &ruin : save.ruins)
            {
                auto entry = cellToJson(ruin.at);
                entry["kind"] =
                    std::string(buildingKindName(ruin.kind));
                entry["state"] = std::string(ruinStateName(ruin.state));
                entry["ticksUntilOut"] = ruin.ticksUntilOut;

                document["ruins"].push_back(std::move(entry));
            }
        }

        for (std::size_t index = 0; index < save.walkers.size(); ++index)
        {
            const auto &fireCall = save.walkers[index].fireCall;

            if (fireCall.has_value())
            {
                document.at("walkers").at(index)["fireCall"] = *fireCall;
            }
        }
    }

    void ruinsFromJson(const nlohmann::json &document, SaveGame &save)
    {
        if (document.contains("ruins"))
        {
            for (const auto &ruin : document.at("ruins"))
            {
                save.ruins.push_back(SavedRuin{
                    .at = cellFromJson(ruin),
                    .kind = ruinKindFromJson(
                        ruin.at("kind").get<std::string>()),
                    .state = ruinStateFromJson(
                        ruin.at("state").get<std::string>()),
                    .ticksUntilOut =
                        ruin.at("ticksUntilOut").get<std::int32_t>(),
                });
            }
        }

        std::size_t index = 0;

        for (const auto &walker : document.at("walkers"))
        {
            save.walkers[index].fireCall =
                linkFromJson(walker, "fireCall");
            ++index;
        }
    }

    void requireConsistentFireCalls(const SaveGame &save)
    {
        for (const auto &walker : save.walkers)
        {
            if (!walker.fireCall.has_value())
            {
                continue;
            }

            if (*walker.fireCall >= save.ruins.size())
            {
                throw SaveFormatError(
                    "antwika::game: a save names a fire call whose "
                    "ruin is not a ruin in it");
            }
        }
    }

} // namespace antwika::game
