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

        FaceUv getUvWithinFace(
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

        [[nodiscard]] std::size_t getSideTowards(const VoxelPosition wayPosition)
        {
            for (std::size_t side = 0; side < kFaces; ++side)
            {
                if (kVoxelFaces[side].neighbourOffsetPosition == wayPosition)
                {
                    return side;
                }
            }

            return 0;
        }

        [[nodiscard]] glm::vec3 getPointWithin(
            const VoxelPosition climbPosition,
            const float alongDistance,
            const float acrossDistance,
            const float upDistance)
        {
            const auto acrossStep = VoxelPosition{.x = -climbPosition.z,
            .z = climbPosition.x};

            return glm::vec3{
                (static_cast<float>(climbPosition.x) * alongDistance)
                    + (static_cast<float>(acrossStep.x) * acrossDistance),
                upDistance,
                (static_cast<float>(climbPosition.z) * alongDistance)
                    + (static_cast<float>(acrossStep.z) * acrossDistance)};
        }

        [[nodiscard]] glm::vec3 getUponFace(
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
            const auto uv = getUvWithinFace(face, one, two);
            const auto push = face.normal * uv.depth;

            return StairQuad{
                .side = side,
                .corners = {
                    getUponFace(face, uv.leastU, uv.leastV) - push,
                    getUponFace(face, uv.mostU, uv.leastV) - push,
                    getUponFace(face, uv.mostU, uv.mostV) - push,
                    getUponFace(face, uv.leastU, uv.mostV) - push}};
        }

    }

    std::vector<StairQuad> getStairQuads(const VoxelPosition climbPosition)
    {
        const auto acrossStep =
            VoxelPosition{.x = -climbPosition.z, .z = climbPosition.x};
        const auto backStep =
            VoxelPosition{.x = -climbPosition.x, .z = -climbPosition.z};

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
                    getSideTowards(VoxelPosition{.y = 1}),
                    getPointWithin(climbPosition, nearHeight, -kHalf, nearHeight),
                    getPointWithin(climbPosition, farHeight, kHalf, nearHeight)));

            quads.push_back(
                quadOf(
                    getSideTowards(backStep),
                    getPointWithin(climbPosition, farHeight, -kHalf, nearHeight),
                    getPointWithin(climbPosition, farHeight, kHalf, farHeight)));

            if (step == 0)
            {
                continue;
            }

            quads.push_back(
                quadOf(
                    getSideTowards(acrossStep),
                    getPointWithin(climbPosition, nearHeight, kHalf, -kHalf),
                    getPointWithin(climbPosition, farHeight, kHalf, nearHeight)));

            quads.push_back(
                quadOf(
                    getSideTowards(
                        VoxelPosition{
                            .x = -acrossStep.x, .z = -acrossStep.z}),
                    getPointWithin(climbPosition, nearHeight, -kHalf, -kHalf),
                    getPointWithin(climbPosition, farHeight, -kHalf, nearHeight)));
        }

        for (const auto axis : {climbPosition, VoxelPosition{.y = -1}})
        {
            const auto side = getSideTowards(axis);

            quads.push_back(
                StairQuad{
                    .side = side,
                    .corners = kVoxelFaces[side].corners});
        }

        return quads;
    } // GCOVR_EXCL_LINE

}
