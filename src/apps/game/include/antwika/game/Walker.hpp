#pragma once

#include <cstdint>
#include <optional>

#include "antwika/game/Direction.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    /**
     * @brief What a walker is carrying, and for whom.
     *
     * It is declared here rather than beside WalkerView because the
     * component is the truth and the view is a copy of it; a snapshot
     * that named its own kind would be a second enumeration to keep in
     * step.  SceneSnapshot.hpp includes this header, so anything drawing
     * a frame still sees it.
     */
    enum class WalkerKind : std::uint8_t
    {
        Food,
        Water,
        Fireman,
        Architect,
    };

    /**
     * @brief How far a walker may go before it is gone.
     *
     * Counted in steps taken rather than in distance from where it
     * started, so a walker going round in circles is as tired as one
     * going in a straight line.
     */
    inline constexpr std::int32_t kMaxWalkDistance = 32;

    /**
     * @brief How much a walker that carries anything sets out with.
     */
    inline constexpr std::int32_t kWalkerCarryCapacity = 100;

    /**
     * @brief Get which resource a walker delivers.
     * @param kind The kind of walker.
     * @return The resource it hands out, or nullopt for a fireman or an
     * architect, who carry nothing and relieve a risk instead.
     */
    [[nodiscard]] constexpr std::optional<Resource> carriedResource(
        WalkerKind kind) noexcept
    {
        if (kind == WalkerKind::Fireman || kind == WalkerKind::Architect)
        {
            return std::nullopt;
        }

        // The two carrying kinds list the two resources in one order.
        // So arithmetic rather than a switch, as turnRight() does.
        return static_cast<Resource>(static_cast<std::uint8_t>(kind));
    }

    static_assert(carriedResource(WalkerKind::Food) == Resource::Food);
    static_assert(carriedResource(WalkerKind::Water) == Resource::Water);

    /**
     * @brief Something that walks the paths, and which way it is facing.
     *
     * Where it is lives in a separate Cell component, so a walker and a
     * path tile are told apart by which components they carry rather than
     * by a flag inside one shared type.
     */
    struct Walker
    {
        Direction facing = Direction::East;
        WalkerKind kind = WalkerKind::Food;

        /** @brief How much of its resource is left to hand out. */
        std::int32_t carried = 0;

        /** @brief How many cells it has moved since it appeared. */
        std::int32_t stepsTaken = 0;

        /**
         * @brief Compare two walkers.
         * @param other The walker to compare against.
         * @return True when the facing, the kind, the load and the
         * distance walked all match.
         */
        [[nodiscard]] bool operator==(const Walker &other) const = default;
    };

    /**
     * @brief Get a walker as it is the moment it appears.
     * @param kind The kind of walker to send out.
     * @param facing The direction it leaves in.
     * @return The walker's starting state, loaded if it carries anything.
     */
    [[nodiscard]] constexpr Walker newlySpawned(
        WalkerKind kind, Direction facing) noexcept
    {
        const auto carries = carriedResource(kind).has_value();

        return Walker{
            .facing = facing,
            .kind = kind,
            .carried = carries ? kWalkerCarryCapacity : 0,
            .stepsTaken = 0};
    }

} // namespace antwika::game
