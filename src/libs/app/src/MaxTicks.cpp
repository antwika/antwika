#include "antwika/app/MaxTicks.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

namespace antwika::app
{

    std::optional<antwika::time::Tick> maxTicksOf(
        const std::optional<std::string_view> value,
        const std::optional<antwika::time::Tick> fallback)
    {
        if (!value.has_value())
        {
            return fallback;
        }

        std::uint64_t given = 0;
        const auto read = std::from_chars(
            value->data(), value->data() + value->size(), given);

        if (read.ec != std::errc{}
            || read.ptr != value->data() + value->size())
        {
            return fallback;
        }

        return given == 0 ? std::optional<antwika::time::Tick>{}
                          : std::optional{given};
    }

}
