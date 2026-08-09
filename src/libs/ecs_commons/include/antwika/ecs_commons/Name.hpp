#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::ecs_commons
{

    inline constexpr std::size_t kNameMaxLength = 31;

    struct Name final
    {
        std::array<char, kNameMaxLength> text{};

        [[nodiscard]] bool operator==(const Name &other) const = default;
    };

    [[nodiscard]] constexpr Name makeName(std::string_view text) noexcept
    {
        Name name{};
        const auto length =
            text.size() < kNameMaxLength ? text.size() : kNameMaxLength;

        for (std::size_t i = 0; i < length; ++i)
        {
            name.text[i] = text[i];
        }

        return name;
    }

    [[nodiscard]] constexpr std::string_view view(const Name &name) noexcept
    {
        std::size_t length = 0;
        while (length < kNameMaxLength && name.text[length] != '\0')
        {
            ++length;
        }

        return std::string_view(name.text.data(), length);
    }

}
