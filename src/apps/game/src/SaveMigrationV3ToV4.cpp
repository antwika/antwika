#include "SaveMigrationV3ToV4.hpp"

#include <cstddef>
#include <string_view>

namespace antwika::game
{

    namespace
    {
        // How many services a version 4 file counts coverage for.
        // Written out rather than read from kServiceCount.
        // A migration describes the format it produced at the time.
        // Never the one this build happens to hold now.
        constexpr std::size_t kVersionFourServiceCount = 2;
    } // namespace

    std::uint32_t DropRiskServices::fromVersion() const noexcept
    {
        return 3;
    }

    std::uint32_t DropRiskServices::toVersion() const noexcept
    {
        return 4;
    }

    // MigrationChain asks for this in one place only.
    // It is the message thrown when a migration is not one step.
    // This one reads 3 and produces 4.
    // So reaching it means editing the two functions above.
    // Which breaks the migration rather than feeding it input.
    // See docs/confirming-unreachable-branches.md.
    // GCOVR_EXCL_START
    std::string_view DropRiskServices::name() const noexcept
    {
        return "drop the risk services from coverage";
    }
    // GCOVR_EXCL_STOP

    void DropRiskServices::apply(nlohmann::json &document) const
    {
        if (!document.contains("buildings")
            || !document.at("buildings").is_array())
        {
            return;
        }

        for (auto &building : document.at("buildings"))
        {
            if (!building.contains("coverage")
                || !building.at("coverage").is_array())
            {
                continue;
            }

            auto &coverage = building.at("coverage");

            // Water and health kept the slots they had.
            // So a truncation is the whole of the rewrite.
            while (coverage.size() > kVersionFourServiceCount)
            {
                coverage.erase(coverage.size() - 1);
            }
        }
    }

} // namespace antwika::game
