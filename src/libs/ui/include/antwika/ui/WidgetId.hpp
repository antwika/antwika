#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace antwika::ui
{

    enum class WidgetId : std::uint64_t
    {
    };

    inline constexpr WidgetId kNoWidget{0};

    template <std::same_as<WidgetId>... Ids>
    [[nodiscard]] constexpr bool assertDistinct(Ids... ids) noexcept
    {
        const std::array<WidgetId, sizeof...(Ids)> values{ids...};

        for (std::size_t index = 0; index < values.size(); ++index)
        {
            for (std::size_t other = index + 1; other < values.size();
                 ++other)
            {
                if (values[index] == values[other])
                {
                    return false;
                }
            }
        }

        return true;
    }

}
