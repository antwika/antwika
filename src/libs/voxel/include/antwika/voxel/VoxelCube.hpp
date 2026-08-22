#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/voxel/VoxelMaterial.hpp"
#include "antwika/voxel/VoxelPosition.hpp"
#include "antwika/voxel/Voxels.hpp"

namespace antwika::voxel
{

    inline constexpr std::int32_t kCubeSide = 2;

    [[nodiscard]] std::int32_t cubeTop(std::int32_t cube);

    [[nodiscard]] std::int32_t cubeIndexOfLevel(std::int32_t level);

    inline constexpr std::size_t kCubeVoxels =
        static_cast<std::size_t>(kCubeSide * kCubeSide * kCubeSide);

    enum class Side : std::uint8_t
    {
        Top,
        Bottom,
        Left,
        Right,
    };

    inline constexpr std::size_t kFaceSides = 4;

    inline constexpr std::array<Side, kFaceSides> kEverySide{
        Side::Top, Side::Bottom, Side::Left, Side::Right};

    enum class EdgeKind : std::uint8_t
    {
        Boundary,
        Interior,
    };

    enum class Corner : std::uint8_t
    {
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    inline constexpr std::size_t kFaceCorners = 4;

    inline constexpr std::array<Corner, kFaceCorners> kEveryCorner{
        Corner::TopLeft,
        Corner::TopRight,
        Corner::BottomLeft,
        Corner::BottomRight};

    struct FaceEdge final
    {
        Side side = Side::Top;

        EdgeKind edge = EdgeKind::Boundary;

        [[nodiscard]] bool operator==(const FaceEdge &other) const
            = default;

        [[nodiscard]] auto operator<=>(const FaceEdge &other) const
            = default;
    };

    [[nodiscard]] Side facing(Side side);

    [[nodiscard]] FaceEdge facing(FaceEdge edge);

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

    [[nodiscard]] std::vector<VoxelPosition> cubeCells(
        VoxelPosition cornerPosition);

    [[nodiscard]] Voxels expandCubesToVoxels(const Voxels &cubeVoxels);

    [[nodiscard]] VoxelPosition rampDirectionFor(
        const Voxels &filledVoxels, VoxelPosition position);

    [[nodiscard]] Voxels cubeVoxels(
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

    [[nodiscard]] Voxels withRampsRebuilt(
        const Voxels &filledVoxels, VoxelPosition position);

}
