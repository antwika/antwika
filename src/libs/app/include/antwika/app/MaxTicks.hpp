#pragma once

#include <optional>
#include <string_view>

#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    inline constexpr std::string_view kMaxTicksFlag = "--max-ticks";

    [[nodiscard]] std::optional<antwika::time::Tick> maxTicksOf(
        std::optional<std::string_view> value,
        std::optional<antwika::time::Tick> fallback);

}
