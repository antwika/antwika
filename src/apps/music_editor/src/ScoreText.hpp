#pragma once

#include <cstdint>
#include <string_view>

namespace antwika::music_editor::detail
{

    inline constexpr std::int32_t kSemitonesPerOctave = 12;

    [[nodiscard]] inline std::string_view trimmed(
        const std::string_view text) noexcept
    {
        constexpr std::string_view kBlanks{" \t"};

        const auto first = text.find_first_not_of(kBlanks);

        if (first == std::string_view::npos)
        {
            return {};
        }

        return text.substr(
            first, text.find_last_not_of(kBlanks) - first + 1);
    }

}
