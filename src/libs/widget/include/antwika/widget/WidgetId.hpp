#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>

namespace antwika::widget
{

    enum class WidgetId : std::uint64_t
    {
    };

    inline constexpr WidgetId kNoWidget{0};

    template <std::size_t Count>
    [[nodiscard]] constexpr bool allDistinct(
        const std::array<WidgetId, Count> &widgetIds) noexcept
    {
        for (std::size_t index = 0; index < widgetIds.size(); ++index)
        {
            for (std::size_t other = index + 1; other < widgetIds.size();
                 ++other)
            {
                if (widgetIds[index] == widgetIds[other])
                {
                    return false;
                }
            }
        }

        return true;
    }

    template <std::same_as<WidgetId>... Ids>
    [[nodiscard]] constexpr bool allDistinct(Ids... ids) noexcept
    {
        return allDistinct(std::array<WidgetId, sizeof...(Ids)>{ids...});
    }

}
