#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "antwika/tileset/PixelClass.hpp"

namespace antwika::tileset
{

    inline constexpr std::int32_t kSpriteSide = 8;

    inline constexpr std::int32_t kSpritePixels =
        kSpriteSide * kSpriteSide;

    inline constexpr std::uint8_t kMaxFrames = 4;

    inline constexpr std::uint8_t kMinWeight = 1;

    inline constexpr std::uint8_t kMaxWeight = 16;

    inline constexpr std::uint8_t kDefaultWeight = 4;

    using SpriteId = std::uint32_t;

    using SocketId = std::uint16_t;

    /**
     * @brief The reserved "edge" socket, carried by base-sprite edges
     *        that face outside the terrain region.
     */
    inline constexpr SocketId kEdgeSocket = 0;

    /**
     * @brief The reserved "open" socket, which an empty cell presents
     *        on decor layers.
     */
    inline constexpr SocketId kOpenSocket = 1;

    enum class Side : std::uint8_t
    {
        North = 0,
        East,
        South,
        West,
    };

    [[nodiscard]] constexpr Side enumBound(Side) noexcept
    {
        return Side::West;
    }

    [[nodiscard]] std::string_view toString(Side side) noexcept;

    struct SpriteFrame final
    {
        std::array<PixelClass, kSpritePixels> pixels{};

        [[nodiscard]] bool operator==(
            const SpriteFrame &other) const = default;
    };

    struct Sprite final
    {
        SpriteId id = 0;
        std::uint8_t frameCount = 1;

        /**
         * @brief The relative frequency of this sprite among the
         *        valid candidates at a cell.
         *
         * Requires: between kMinWeight and kMaxWeight inclusive.
         */
        std::uint8_t weight = kDefaultWeight;
        std::array<SpriteFrame, kMaxFrames> frames{};
        std::array<SocketId, 4> sockets{
            kOpenSocket, kOpenSocket, kOpenSocket, kOpenSocket};

        /**
         * @brief The base sprite ids this decor sprite may sit on.
         *
         * Requires: empty on layer 0.
         */
        std::vector<SpriteId> on{};

        [[nodiscard]] bool operator==(
            const Sprite &other) const = default;
    };

    /**
     * @brief Whether a frame carries no drawable pixel.
     *
     * @param frame The frame to inspect.
     * @return Whether every pixel of it is PixelClass::Blank.
     *
     * Ensures: a default-constructed frame is blank.
     */
    [[nodiscard]] bool isBlank(const SpriteFrame &frame) noexcept;

}
