#include <cstddef>
#include <cstdint>

#include <nlohmann/json.hpp>

#include "antwika/game/SaveGame.hpp"
#include "SaveSections.hpp"

/**
 * @file
 * @brief The save document's labour section.
 *
 * One optional count per building, saying how many of the city's people
 * were working there. Absent means nobody has been allocated to it yet,
 * which is what a version-3 file written before labour existed says and
 * what a workplace put up this tick holds -- so this is additive per
 * docs/schema-versioning.md and carries no migration.
 *
 * **How many workers a building *wanted* is not written**, because
 * workersWantedBy() answers it from the kind the file already names. A
 * copy of it here would be a second truth that a file could disagree with
 * its own building about, and a save that disagreed with itself is a
 * session somebody never had.
 *
 * **Neither the workforce total nor any rating is written either**, for
 * the same reason: both are sums over what the file already holds -- see
 * CityRatings.
 */
namespace antwika::game
{

    void describeLabour(nlohmann::json &building)
    {
        building["properties"]["employed"] = signedCountShape();
    }

    void labourToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &employed = save.buildings[index].employed;

            if (!employed.has_value())
            {
                continue;
            }

            document.at("buildings").at(index)["employed"] = *employed;
        }
    }

    void labourFromJson(const nlohmann::json &document, SaveGame &save)
    {
        std::size_t index = 0;

        for (const auto &building : document.at("buildings"))
        {
            if (building.contains("employed"))
            {
                save.buildings[index].employed =
                    building.at("employed").get<std::int32_t>();
            }

            ++index;
        }
    }

} // namespace antwika::game
