#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <nlohmann/json.hpp>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/Coverage.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/Service.hpp"
#include "SaveSections.hpp"

/**
 * @file
 * @brief The save document's coverage section.
 *
 * One optional array per building, in Service order, holding the ticks
 * of each service still reaching it. Absent means every one of them is
 * zero, which is both what a building nothing has covered holds and
 * what a version-3 file written before coverage existed says -- so this
 * is additive per docs/schema-versioning.md and carries no migration.
 *
 * Bounded by kCoverageFull rather than by what an int32 holds, unlike
 * every other count in this format. The bound is not arithmetic
 * hygiene: a countdown above the full one would take longer to run out
 * than any walker could ever produce, so a file naming one is a session
 * nobody played, and this project refuses one rather than clamping it.
 */
namespace antwika::game
{

    namespace
    {
        [[nodiscard]] bool anyCovered(
            const std::array<std::int32_t, kServiceCount> &coverage)
        {
            return std::ranges::any_of(
                coverage, [](std::int32_t left) { return left != 0; });
        }
    } // namespace

    void describeCoverage(nlohmann::json &building)
    {
        building["properties"]["coverage"]["type"] = "array";
        building["properties"]["coverage"]["items"] =
            replay::boundedCountShape(kCoverageFull);
        building["properties"]["coverage"]["minItems"] = kServiceCount;
        building["properties"]["coverage"]["maxItems"] = kServiceCount;
    }

    void coverageToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            // Written only where there is something to say.
            // An all-zero array and an absent member read the same.
            // So the smaller file is the one worth writing.
            if (!anyCovered(save.buildings[index].coverage))
            {
                continue;
            }

            document["buildings"][index]["coverage"] =
                save.buildings[index].coverage;
        }
    }

    void coverageFromJson(const nlohmann::json &document, SaveGame &save)
    {
        const auto &buildings = document.at("buildings");

        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &building = buildings.at(index);

            if (!building.contains("coverage"))
            {
                continue;
            }

            for (std::size_t slot = 0; slot < kServiceCount; ++slot)
            {
                save.buildings[index].coverage[slot] =
                    building.at("coverage").at(slot).get<std::int32_t>();
            }
        }
    }

} // namespace antwika::game
