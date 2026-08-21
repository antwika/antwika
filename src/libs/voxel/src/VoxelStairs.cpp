#include "antwika/voxel/VoxelStairs.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cstddef>

#include "antwika/voxel/VoxelDetail.hpp"

namespace antwika::voxel
{
    using namespace detail;

    namespace detail
    {

        FaceUv uvWithinFace(
            const Face &face, const glm::vec3 one, const glm::vec3 two)
        {
            const auto acrossVector = face.corners[1] - face.corners[0];
            const auto downVector = face.corners[3] - face.corners[0];
            const auto share =
                [](const glm::vec3 axis, const glm::vec3 vector)
            { return glm::dot(vector, axis) / glm::dot(axis, axis); };
            const auto hereVector = one - face.corners[0];
            const auto thereVector = two - face.corners[0];
            const auto oneU = share(acrossVector, hereVector);
            const auto twoU = share(acrossVector, thereVector);
            const auto oneV = share(downVector, hereVector);
            const auto twoV = share(downVector, thereVector);

            return FaceUv{
                .leastU = std::min(oneU, twoU),
                .mostU = std::max(oneU, twoU),
                .leastV = std::min(oneV, twoV),
                .mostV = std::max(oneV, twoV),
                .depth = -glm::dot(hereVector, face.normal)};
        }

    }

    namespace
    {

        [[nodiscard]] std::size_t sideTowards(const VoxelCell wayCell)
        {
            for (std::size_t side = 0; side < kFaces; ++side)
            {
                const auto &neighbourOffset =
                    kVoxelFaces[side].neighbourOffsetCell;

                if (neighbourOffset.x == wayCell.x
                    && neighbourOffset.y == wayCell.y
                    && neighbourOffset.z == wayCell.z)
                {
                    return side;
                }
            }

            return 0;
        }

        [[nodiscard]] glm::vec3 within(
            const VoxelCell climbCell,
            const float alongDistance,
            const float acrossDistance,
            const float upDistance)
        {
            const auto acrossCell = VoxelCell{.x = -climbCell.z,
            .z = climbCell.x};

            return glm::vec3{
                (static_cast<float>(climbCell.x) * alongDistance)
                    + (static_cast<float>(acrossCell.x) * acrossDistance),
                upDistance,
                (static_cast<float>(climbCell.z) * alongDistance)
                    + (static_cast<float>(acrossCell.z) * acrossDistance)};
        }

        [[nodiscard]] glm::vec3 uponFace(
            const Face &face, const float u, const float v)
        {
            const auto acrossVector = face.corners[1] - face.corners[0];
            const auto downVector = face.corners[3] - face.corners[0];

            return face.corners[0] + (acrossVector * u) + (downVector * v);
        }

        [[nodiscard]] StairQuad quadOf(
            const std::size_t side,
            const glm::vec3 one,
            const glm::vec3 two)
        {
            const auto &face = kVoxelFaces[side];
            const auto uv = uvWithinFace(face, one, two);
            const auto push = face.normal * uv.depth;

            return StairQuad{
                .side = side,
                .corners = {
                    uponFace(face, uv.leastU, uv.leastV) - push,
                    uponFace(face, uv.mostU, uv.leastV) - push,
                    uponFace(face, uv.mostU, uv.mostV) - push,
                    uponFace(face, uv.leastU, uv.mostV) - push}};
        }

    }

    std::vector<StairQuad> stairQuads(const VoxelCell climbCell)
    {
        const auto acrossCell = VoxelCell{.x = -climbCell.z, .z = climbCell.x};
        const auto backCell = VoxelCell{.x = -climbCell.x, .z = -climbCell.z};

        std::vector<StairQuad> quads;

        quads.reserve(kStairQuads);

        for (std::size_t step = 0; step < kStepsPerVoxel; ++step)
        {
            const auto nearHeight =
                -kHalf
                + (static_cast<float>(step) * kStepHeightFraction);
            const auto farHeight = nearHeight + kStepHeightFraction;

            quads.push_back(
                quadOf(
                    sideTowards(VoxelCell{.y = 1}),
                    within(climbCell, nearHeight, -kHalf, nearHeight),
                    within(climbCell, farHeight, kHalf, nearHeight)));

            quads.push_back(
                quadOf(
                    sideTowards(backCell),
                    within(climbCell, farHeight, -kHalf, nearHeight),
                    within(climbCell, farHeight, kHalf, farHeight)));

            if (step == 0)
            {
                continue;
            }

            quads.push_back(
                quadOf(
                    sideTowards(acrossCell),
                    within(climbCell, nearHeight, kHalf, -kHalf),
                    within(climbCell, farHeight, kHalf, nearHeight)));

            quads.push_back(
                quadOf(
                    sideTowards(
                        VoxelCell{.x = -acrossCell.x, .z = -acrossCell.z}),
                    within(climbCell, nearHeight, -kHalf, -kHalf),
                    within(climbCell, farHeight, -kHalf, nearHeight)));
        }

        for (const auto axis : {climbCell, VoxelCell{.y = -1}})
        {
            const auto side = sideTowards(axis);

            quads.push_back(
                StairQuad{
                    .side = side,
                    .corners = kVoxelFaces[side].corners});
        }

        return quads;
    } // GCOVR_EXCL_LINE

}
