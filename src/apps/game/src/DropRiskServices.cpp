#include "DropRiskServices.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <string_view>

namespace antwika::game
{

    namespace
    {
        constexpr std::size_t kVersionFourServiceCount = 2;
    }

    std::uint32_t DropRiskServices::fromVersion() const noexcept
    {
        return 3;
    }

    std::uint32_t DropRiskServices::toVersion() const noexcept
    {
        return 4;
    }

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

            while (coverage.size() > kVersionFourServiceCount)
            {
                coverage.erase(coverage.size() - 1);
            }
        }
    }

}
