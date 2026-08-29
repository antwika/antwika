#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/enums/Enumeration.hpp>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/Voxels.hpp"

namespace antwika::voxel
{

    inline constexpr std::int32_t kCubeSide = 2;

    [[nodiscard]] std::int32_t getCubeTop(std::int32_t cube);

    [[nodiscard]] std::int32_t getCubeIndexOfLevel(std::int32_t level);

    inline constexpr std::size_t kCubeVoxels =
        static_cast<std::size_t>(kCubeSide * kCubeSide * kCubeSide);

    enum class Side : std::uint8_t
    {
        Top,
        Bottom,
        Left,
        Right,
    };

    [[nodiscard]] constexpr Side getLastEnumerator(Side) noexcept
    {
        return Side::Right;
    }

    inline constexpr std::size_t kFaceSides = enums::kCount<Side>;

    inline constexpr std::array<Side, kFaceSides> kEverySide =
        enums::kAll<Side>;

    enum class EdgeKind : std::uint8_t
    {
        Boundary,
        Interior,
    };

    [[nodiscard]] constexpr EdgeKind getLastEnumerator(EdgeKind) noexcept
    {
        return EdgeKind::Interior;
    }

    enum class Corner : std::uint8_t
    {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    [[nodiscard]] constexpr Corner getLastEnumerator(Corner) noexcept
    {
        return Corner::BottomRight;
    }

    inline constexpr std::size_t kFaceCorners = enums::kCount<Corner>;

    inline constexpr std::array<Corner, kFaceCorners> kEveryCorner =
        enums::kAll<Corner>;

    struct FaceEdge final
    {
        Side side = Side::Top;

        EdgeKind edge = EdgeKind::Boundary;

        [[nodiscard]] bool operator==(const FaceEdge &other) const
            = default;

        [[nodiscard]] auto operator<=>(const FaceEdge &other) const
            = default;
    };

    [[nodiscard]] Side getFacing(Side side);

    [[nodiscard]] FaceEdge getFacing(FaceEdge edge);

    inline constexpr std::size_t kFaceEdges = 8;

    inline constexpr std::array<FaceEdge, kFaceEdges> kEveryFaceEdge{
        FaceEdge{.side = Side::Top, .edge = EdgeKind::Boundary},
        FaceEdge{.side = Side::Top, .edge = EdgeKind::Interior},
        FaceEdge{.side = Side::Bottom, .edge = EdgeKind::Boundary},
        FaceEdge{.side = Side::Bottom, .edge = EdgeKind::Interior},
        FaceEdge{.side = Side::Left, .edge = EdgeKind::Boundary},
        FaceEdge{.side = Side::Left, .edge = EdgeKind::Interior},
        FaceEdge{.side = Side::Right, .edge = EdgeKind::Boundary},
        FaceEdge{.side = Side::Right, .edge = EdgeKind::Interior}};

    [[nodiscard]] VoxelPosition cubeCornerOf(VoxelPosition position);

    /**
     * @brief The cube a voxel belongs to, counted in whole cubes, so that
     * one step of the count is one cube of the pile.
     */
    [[nodiscard]] VoxelPosition cubeIndexOf(VoxelPosition position);

    /**
     * @brief The voxel a cube of the count corners on, the way back from
     * cubeIndexOf.
     */
    [[nodiscard]] VoxelPosition cubeCornerAt(VoxelPosition cubePosition);

    [[nodiscard]] std::vector<VoxelPosition> getCubeCells(
        VoxelPosition cornerPosition);

    [[nodiscard]] Voxels getExpandCubesToVoxels(const Voxels &cubeVoxels);

    [[nodiscard]] VoxelPosition rampDirectionFor(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] Voxels getCubeVoxels(
        VoxelPosition cornerPosition,
        Kind kind,
        VoxelPosition climbPosition);

    [[nodiscard]] Voxels withBlockAt(
        const Voxels &filledVoxels,
        VoxelPosition position,
        Kind kind = Kind::Normal,
        Facing facingOverride = Facing::Any);

    [[nodiscard]] Voxels withoutBlockAt(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] Voxels getWithRampsRebuilt(
        const Voxels &filledVoxels, VoxelPosition position);

}
