#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace antwika::enums
{

    namespace detail
    {
        template <typename Enum, std::size_t... Index>
        [[nodiscard]] constexpr std::array<Enum, sizeof...(Index)> listOf(
            std::index_sequence<Index...>) noexcept
        {
            return {{static_cast<Enum>(Index)...}};
        }
    }

    template <typename Enum>
    inline constexpr std::size_t kCount =
        static_cast<std::size_t>(enumBound(Enum{})) + 1;

    template <typename Enum>
    [[nodiscard]] constexpr std::size_t index(const Enum value) noexcept
    {
        return static_cast<std::size_t>(value);
    }

    template <typename Enum>
    [[nodiscard]] constexpr Enum at(const std::size_t index) noexcept
    {
        return static_cast<Enum>(index % kCount<Enum>);
    }

    template <typename Enum>
    inline constexpr std::array<Enum, kCount<Enum>> kAll =
        detail::listOf<Enum>(std::make_index_sequence<kCount<Enum>>{});

    template <typename Enum, typename Value>
    [[nodiscard]] constexpr const Value &pick(
        const std::array<Value, kCount<Enum>> &table,
        const Enum value) noexcept
    {
        return table[index(value) % kCount<Enum>];
    }

}
