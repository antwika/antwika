#include <cstddef>
#include <cstdint>

#include <nlohmann/json.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "SaveSections.hpp"

/**
 * @file
 * @brief The save document's labour section.
 *
 * Two optional ledgers per building, saying who worked there and where
 * this house's people worked, by index into the buildings array --
 * SavedWalker::home's convention, for its reason. Absent means nobody,
 * which is what a file written before labour walked says, so both are
 * additive per docs/schema-versioning.md and carry no migration.
 *
 * **The legacy "employed" count is still described and deliberately
 * ignored.** A file that carries it stays valid, but a bare count
 * without the houses it came from is a ledger nobody can decay
 * honestly, so such a city loads unstaffed and staffs itself again.
 *
 * **How many workers a building *wanted* is still not written**:
 * workersWantedBy() answers it from the kind the file already names.
 */
namespace antwika::game
{

    namespace
    {
        [[nodiscard]] nlohmann::json ledgerShape(
            const char *linkName)
        {
            nlohmann::json entry;
            entry["type"] = "object";
            entry["additionalProperties"] = false;
            entry["required"] = {linkName, "count"}; // GCOVR_EXCL_LINE
            entry["properties"][linkName] = signedCountShape();
            entry["properties"]["count"] = signedCountShape();

            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;
            // GCOVR_EXCL_START
            shape["required"] = {"entries", "countdown"};
            // GCOVR_EXCL_STOP
            shape["properties"]["entries"]["type"] = "array";
            shape["properties"]["entries"]["items"] = entry;
            shape["properties"]["countdown"] = signedCountShape();
            return shape;
        } // GCOVR_EXCL_LINE
    } // namespace

    void describeLabour(nlohmann::json &building)
    {
        building["properties"]["employed"] = signedCountShape();
        building["properties"]["staff"] = ledgerShape("house");
        building["properties"]["employment"] = ledgerShape("workplace");
    }

    void labourToJson(const SaveGame &save, nlohmann::json &document)
    {
        for (std::size_t index = 0; index < save.buildings.size(); ++index)
        {
            const auto &saved = save.buildings[index];

            if (saved.staff.has_value())
            {
                nlohmann::json ledger;
                ledger["entries"] = nlohmann::json::array();

                for (const auto &entry : saved.staff->entries)
                {
                    nlohmann::json one;
                    one["house"] = entry.house;
                    one["count"] = entry.count;
                    ledger["entries"].push_back(one);
                }

                ledger["countdown"] = saved.staff->ticksUntilDecay;
                document.at("buildings").at(index)["staff"] = ledger;
            }

            if (saved.employment.has_value())
            {
                nlohmann::json ledger;
                ledger["entries"] = nlohmann::json::array();

                for (const auto &job : saved.employment->jobs)
                {
                    nlohmann::json one;
                    one["workplace"] = job.workplace;
                    one["count"] = job.count;
                    ledger["entries"].push_back(one);
                }

                ledger["countdown"] =
                    saved.employment->ticksUntilDispatch;
                document.at("buildings").at(index)["employment"] =
                    ledger;
            }
        }
    }

    void labourFromJson(const nlohmann::json &document, SaveGame &save)
    {
        std::size_t index = 0;

        for (const auto &building : document.at("buildings"))
        {
            if (building.contains("staff"))
            {
                const auto &ledger = building.at("staff");
                // The unwind pad of a record with a vector member.
                // See docs/confirming-unreachable-branches.md, (a).
                // GCOVR_EXCL_START
                StoredStaff staff{
                    .entries = {},
                    .ticksUntilDecay =
                        ledger.at("countdown").get<std::int32_t>()};
                // GCOVR_EXCL_STOP

                for (const auto &entry : ledger.at("entries"))
                {
                    staff.entries.push_back(
                        StoredStaffEntry{
                            .house =
                                entry.at("house").get<std::size_t>(),
                            .count =
                                entry.at("count").get<std::int32_t>()});
                }

                save.buildings[index].staff = staff;
            }

            if (building.contains("employment"))
            {
                const auto &ledger = building.at("employment");
                // The unwind pad of a record with a vector member.
                // See docs/confirming-unreachable-branches.md, (a).
                // GCOVR_EXCL_START
                StoredEmployment employment{
                    .jobs = {},
                    .ticksUntilDispatch =
                        ledger.at("countdown").get<std::int32_t>()};
                // GCOVR_EXCL_STOP

                for (const auto &entry : ledger.at("entries"))
                {
                    employment.jobs.push_back(
                        StoredJob{
                            .workplace = entry.at("workplace")
                                             .get<std::size_t>(),
                            .count =
                                entry.at("count").get<std::int32_t>()});
                }

                save.buildings[index].employment = employment;
            }

            ++index;
        }
    }

    void requireConsistentStaffing(const SaveGame &save)
    {
        for (const auto &building : save.buildings)
        {
            if (building.staff.has_value())
            {
                for (const auto &entry : building.staff->entries)
                {
                    if (entry.house >= save.buildings.size())
                    {
                        throw SaveFormatError(
                            "antwika::game: a staff entry names no "
                            "such building");
                    }
                }
            }

            if (building.employment.has_value())
            {
                for (const auto &job : building.employment->jobs)
                {
                    if (job.workplace >= save.buildings.size())
                    {
                        throw SaveFormatError(
                            "antwika::game: a job holding names no "
                            "such building");
                    }
                }
            }
        }
    }

} // namespace antwika::game
