#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::event
{

    inline constexpr std::string_view kTickText = "engine.tick";

    inline constexpr std::string_view kStopText = "engine.stop";

    inline constexpr std::string_view kKeyDownText = "input.key_down";

    inline constexpr std::string_view kKeyUpText = "input.key_up";

    inline constexpr std::string_view kPointerMoveText = "input.pointer_move";

    inline constexpr std::string_view kPointerDownText = "input.pointer_down";

    inline constexpr std::string_view kPointerUpText = "input.pointer_up";

    inline constexpr std::string_view kPointerScrollText =
        "input.pointer_scroll";

    inline constexpr auto kSeededEventNames = std::to_array<std::string_view>({
        "",
        kTickText,
        kStopText,
        kKeyDownText,
        kKeyUpText,
        kPointerMoveText,
        kPointerDownText,
        kPointerUpText,
        kPointerScrollText,
    });

    namespace detail
    {
        [[nodiscard]] std::uint32_t getUnseededTextFailure(
            std::string_view text);
    }

    [[nodiscard]] consteval std::uint32_t getSeededIdOf(std::string_view text)
    {
        for (std::size_t index = 0; index < kSeededEventNames.size(); ++index)
        {
            if (kSeededEventNames[index] == text)
            {
                return static_cast<std::uint32_t>(index);
            }
        }

        return detail::getUnseededTextFailure(text);
    }

    [[nodiscard]] consteval bool seedsEveryNameOnce()
    {
        const auto count = kSeededEventNames.size();

        for (std::size_t left = 0; left < count; ++left)
        {
            for (std::size_t right = left + 1; right < count; ++right)
            {
                if (kSeededEventNames[left] == kSeededEventNames[right])
                {
                    return false;
                }
            }
        }

        return true;
    }

    static_assert(
        seedsEveryNameOnce(),
        "kSeededEventNames must not seed the same text twice");

}
