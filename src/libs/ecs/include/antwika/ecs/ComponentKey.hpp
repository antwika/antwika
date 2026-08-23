#pragma once

#include <cstdint>
#include <string_view>

#include "antwika/ecs/Component.hpp"

namespace antwika::ecs
{

    using ComponentKey = std::uint64_t;

}

namespace antwika::ecs::detail
{

    template <typename T>
    [[nodiscard]] constexpr std::string_view typeName() noexcept
    {
        constexpr std::string_view signature{__PRETTY_FUNCTION__};
        constexpr std::string_view lead{"T = "};
        constexpr auto nameStart = signature.find(lead) + lead.size();
        constexpr auto nameEnd = signature.find_first_of(";]", nameStart);

        return signature.substr(nameStart, nameEnd - nameStart);
    }

    [[nodiscard]] constexpr ComponentKey getKeyOfName(
        const std::string_view name) noexcept
    {
        constexpr ComponentKey kOffsetBasisKey = 14695981039346656037ULL;
        constexpr ComponentKey kPrimeKey = 1099511628211ULL;
        auto hashKey = kOffsetBasisKey;

        for (const auto letter : name)
        {
            hashKey ^= static_cast<unsigned char>(letter);
            hashKey *= kPrimeKey;
        }

        return hashKey == 0 ? 1 : hashKey;
    }

    void claimComponentKey(ComponentKey key, std::string_view name);

    template <Component T>
    [[nodiscard]] constexpr ComponentKey componentKey() noexcept
    {
        return getKeyOfName(typeName<T>());
    }

}
