#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>

#include <antwika/replay/JsonShapes.hpp>

#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/SaveGame.hpp"
#include "SaveSections.hpp"

namespace antwika::game
{

    namespace
    {
        [[nodiscard]] nlohmann::json ledgerShape(
            const char *linkName)
        {
            nlohmann::json entry =
                antwika::replay::objectShape({linkName, "count"});
            entry["properties"][linkName] = signedCountShape();
            entry["properties"]["count"] = signedCountShape();

            nlohmann::json shape =
                antwika::replay::objectShape({"entries", "countdown"});
            shape["properties"]["entries"]["type"] = "array";
            shape["properties"]["entries"]["items"] = entry;
            shape["properties"]["countdown"] = signedCountShape();
            return shape;
        } // GCOVR_EXCL_LINE
    }

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

}
