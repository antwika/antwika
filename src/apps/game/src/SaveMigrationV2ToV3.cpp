#include "SaveMigrationV2ToV3.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace antwika::game
{

    namespace
    {
        using Rename = std::pair<std::string_view, std::string_view>;

        // Only the names that changed are listed.
        // A house is still a house, and a fire station still one.
        constexpr std::array<Rename, 3> kBuildingRenames{{
            {"food_source", "farm"},
            {"water_source", "well"},
            {"architect_post", "engineer_post"},
        }};

        // A version 2 walker was named for the good it carried.
        // A version 3 one is named for the errand it is on.
        constexpr std::array<Rename, 3> kWalkerRenames{{
            {"food", "market_seller"},
            {"water", "water_carrier"},
            {"architect", "engineer"},
        }};

        // How many goods a version 2 file knew about.
        // Which is exactly how long its stock arrays are.
        constexpr std::size_t kVersionTwoResourceCount = 2;

        // Which of those two was food, and stayed a good.
        constexpr std::size_t kVersionTwoFoodSlot = 0;

        // How many goods a version 3 file knows about.
        // Written out rather than read from kResourceCount.
        // A migration describes the format it produced at the time.
        // Never the one this build happens to hold now.
        constexpr std::size_t kVersionThreeResourceCount = 3;

        void rename(
            nlohmann::json &entry,
            const char *key,
            std::span<const Rename> renames)
        {
            if (!entry.contains(key) || !entry.at(key).is_string())
            {
                return;
            }

            const auto was = entry.at(key).get<std::string>();

            for (const auto &[from, to] : renames)
            {
                if (was == from)
                {
                    entry[key] = std::string(to);
                    return;
                }
            }
        }

        // An absent one is nobody, and so is an explicit null.
        // Version 2 wrote the member only when somebody was out.
        // A hand-written file saying so outright means the same.
        void wrapWalkerLink(nlohmann::json &building)
        {
            if (!building.contains("walker"))
            {
                return;
            }

            const auto held = building.at("walker");
            building.erase("walker");

            if (held.is_null())
            {
                return;
            }

            // Built by hand rather than from an initialiser list.
            // nlohmann reads a two-element list as an object.
            // So that constructor carries a branch this cannot take.
            auto out = nlohmann::json::array();
            out.push_back(held);
            building["walkers"] = std::move(out);
        }

        void widenStock(nlohmann::json &building)
        {
            if (!building.contains("stock")
                || !building.at("stock").is_array()
                || building.at("stock").size()
                    != kVersionTwoResourceCount)
            {
                return;
            }

            const auto food = building.at("stock").at(kVersionTwoFoodSlot);

            auto widened = nlohmann::json::array();
            widened.push_back(food);

            while (widened.size() < kVersionThreeResourceCount)
            {
                widened.push_back(0);
            }

            building["stock"] = std::move(widened);
        }
    } // namespace

    std::uint32_t RenameToServices::fromVersion() const noexcept
    {
        return 2;
    }

    std::uint32_t RenameToServices::toVersion() const noexcept
    {
        return 3;
    }

    // MigrationChain asks for this in one place only.
    // It is the message thrown when a migration is not one step.
    // This one reads 2 and produces 3.
    // So reaching it means editing the two functions above.
    // Which breaks the migration rather than feeding it input.
    // See docs/confirming-unreachable-branches.md.
    // GCOVR_EXCL_START
    std::string_view RenameToServices::name() const noexcept
    {
        return "rename goods to services";
    }
    // GCOVR_EXCL_STOP

    void RenameToServices::apply(nlohmann::json &document) const
    {
        if (document.contains("buildings")
            && document.at("buildings").is_array())
        {
            for (auto &building : document.at("buildings"))
            {
                rename(building, "kind", kBuildingRenames);
                wrapWalkerLink(building);
                widenStock(building);
            }
        }

        if (document.contains("walkers")
            && document.at("walkers").is_array())
        {
            for (auto &walker : document.at("walkers"))
            {
                rename(walker, "kind", kWalkerRenames);
            }
        }
    }

} // namespace antwika::game
