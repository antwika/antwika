#include "antwika/app/ParseMaxTicks.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

namespace antwika::app
{

    std::optional<antwika::time::Tick> getParseMaxTicks(
        const std::optional<std::string_view> value,
        const std::optional<antwika::time::Tick> fallbackTick)
    {
        if (!value.has_value())
        {
            return fallbackTick;
        }

        std::uint64_t tickCount = 0;
        const auto parseResult = std::from_chars(
            value->data(), value->data() + value->size(), tickCount);

        if (parseResult.ec != std::errc{}
            || parseResult.ptr != value->data() + value->size())
        {
            return fallbackTick;
        }

        return tickCount == 0 ? std::optional<antwika::time::Tick>{}
                          : std::optional{tickCount};
    }

}
