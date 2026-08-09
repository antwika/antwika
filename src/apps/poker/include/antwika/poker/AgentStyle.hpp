#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/enums/Enumeration.hpp>

namespace antwika::poker
{

    enum class AgentStyle : std::uint8_t
    {
        Tight = 0,

        Balanced,

        Aggressive,
    };

    [[nodiscard]] constexpr AgentStyle enumBound(AgentStyle) noexcept
    {
        return AgentStyle::Aggressive;
    }

    inline constexpr std::size_t kAgentStyleCount =
        antwika::enums::kCount<AgentStyle>;

    [[nodiscard]] std::string_view toString(AgentStyle style) noexcept;

}
