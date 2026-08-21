#pragma once

#include <cstdint>

namespace antwika::ecs
{

    enum class Entity : std::uint64_t
    {
    };

    inline constexpr Entity kNullEntity{0};

    [[nodiscard]] constexpr std::uint64_t rawValue(Entity entity) noexcept
    {
        return static_cast<std::uint64_t>(entity);
    }

}
