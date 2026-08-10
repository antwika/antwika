#include "RenameToServices.hpp"

#include <nlohmann/json.hpp>

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

        constexpr std::array<Rename, 3> kBuildingRenames{{
            {"food_source", "farm"},
            {"water_source", "well"},
            {"architect_post", "engineer_post"},
        }};

        constexpr std::array<Rename, 3> kWalkerRenames{{
            {"food", "market_seller"},
            {"water", "water_carrier"},
            {"architect", "engineer"},
        }};

        constexpr std::size_t kVersionTwoResourceCount = 2;

        constexpr std::size_t kVersionTwoFoodSlot = 0;

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
    }

    std::uint32_t RenameToServices::fromVersion() const noexcept
    {
        return 2;
    }

    std::uint32_t RenameToServices::toVersion() const noexcept
    {
        return 3;
    }

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

}
