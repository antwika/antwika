#pragma once

#include <cstdint>

namespace antwika::pathfinding
{

    enum class NodeId : std::uint32_t
    {
    };

    [[nodiscard]] constexpr std::uint32_t rawValue(NodeId node) noexcept
    {
        return static_cast<std::uint32_t>(node);
    }

    [[nodiscard]] constexpr NodeId nodeId(std::uint32_t value) noexcept
    {
        return static_cast<NodeId>(value);
    }

}
