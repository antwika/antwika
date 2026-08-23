#pragma once

#include <array>
#include <cstddef>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/voxel/VoxelDetail.hpp>

#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::voxelmap::voxeldetail
{

    using voxel::detail::kFaces;

    using voxel::detail::kCornersPerFace;

    using voxel::detail::kHalf;

    using voxel::detail::Face;

    using voxel::detail::kVoxelFaces;

    using voxel::detail::getOffsetBy;

    using voxel::detail::kindAt;

    using voxel::detail::effectiveKindAt;

    using voxel::detail::materialAt;

    using voxel::detail::isRampStep;

    using voxel::detail::kStepHeightFraction;

    using voxel::detail::FaceUv;

    using voxel::detail::getUvWithinFace;

    constexpr std::size_t kTiles =
        static_cast<std::size_t>(tilemap::kAtlasColumns * tilemap::kAtlasRows);

    constexpr gfx::Color kNoTintColor{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

    constexpr std::array<gfx::Vec2, kCornersPerFace> kFaceCorners{
        gfx::Vec2{0.0F, 1.0F},
        gfx::Vec2{1.0F, 1.0F},
        gfx::Vec2{1.0F, 0.0F},
        gfx::Vec2{0.0F, 0.0F}};

}
