#pragma once

#include <cstdint>

namespace antwika::pathfinding
{

    /**
     * @brief A node in the caller's graph, opaque to this library.
     *
     * The library never interprets the number beyond comparing it, so
     * a caller is free to pack whatever it likes into one -- a flat
     * grid index, a room number, a handle into its own table. Ordering
     * on it is nevertheless load-bearing: it is the final tie-break in
     * the open set, so a caller that renumbers its nodes may get a
     * different, equally cheap path.
     */
    enum class NodeId : std::uint32_t
    {
    };

    /**
     * @brief The number a NodeId carries.
     * @param node The node to unwrap.
     * @return Its underlying value.
     */
    [[nodiscard]] constexpr std::uint32_t rawValue(NodeId node) noexcept
    {
        return static_cast<std::uint32_t>(node);
    }

    /**
     * @brief Wrap a number as a NodeId.
     * @param value The number to wrap.
     * @return The node it names.
     */
    [[nodiscard]] constexpr NodeId nodeId(std::uint32_t value) noexcept
    {
        return static_cast<NodeId>(value);
    }

} // namespace antwika::pathfinding
