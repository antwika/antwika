#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Color.hpp>

namespace antwika::atlas_editor
{

    using antwika::gfx::Color;

    inline constexpr std::size_t kInkChannels = 3;

    inline constexpr std::uint32_t kInkRange = 255;

    inline constexpr std::array<std::string_view, kInkChannels>
        kInkNames{"r", "g", "b"};

    /**
     * @brief Reads one channel of a colour.
     *
     * @param ink The colour to read.
     * @param channel The channel, counted red, green, blue; anything
     *                beyond blue reads as blue.
     * @return The channel's level.
     */
    [[nodiscard]] std::uint8_t inkChannelOf(
        Color ink, std::size_t channel) noexcept;

    /**
     * @brief Replaces one channel of a colour.
     *
     * @param ink The colour to change.
     * @param channel The channel, counted red, green, blue; anything
     *                beyond blue writes blue.
     * @param level The level to set.
     * @return The colour, with that channel at that level.
     */
    [[nodiscard]] Color withInkChannel(
        Color ink, std::size_t channel, std::uint8_t level) noexcept;

}
