#include "antwika/gfx/MeshBox.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>

namespace antwika::gfx
{

    namespace
    {
        constexpr std::size_t kBoxCorners = 8;

        constexpr std::size_t kClipPlanes = 6;
    }

    MeshBox getMeshBox(const MeshData &mesh)
    {
        if (mesh.vertices.empty())
        {
            return MeshBox{};
        }

        auto lowPosition = mesh.vertices.front().position;
        auto highPosition = lowPosition;

        for (const auto &vertex : mesh.vertices)
        {
            lowPosition = glm::min(lowPosition, vertex.position);
            highPosition = glm::max(highPosition, vertex.position);
        }

        return MeshBox{
            .lowPosition = lowPosition, .highPosition = highPosition};
    }

    float getSpanFromBox(const MeshBox box, const Vec3 fromPosition)
    {
        const auto nearestPosition =
            glm::min(glm::max(fromPosition, box.lowPosition), box.highPosition);
        const auto apart = nearestPosition - fromPosition;

        return glm::length(apart);
    }

    bool isBoxBeyond(
        const MeshBox box, const Vec3 fromPosition, const float reach)
    {
        return getSpanFromBox(box, fromPosition) > reach;
    }

    bool isBoxOutside(const MeshBox box, const Mat4 &clipMatrix)
    {
        std::array<bool, kClipPlanes> allBeyond{};

        allBeyond.fill(true);

        for (std::size_t corner = 0; corner < kBoxCorners; ++corner)
        {
            const Vec3 cornerPosition{
                (corner & 1U) == 0U ? box.lowPosition.x : box.highPosition.x,
                (corner & 2U) == 0U ? box.lowPosition.y : box.highPosition.y,
                (corner & 4U) == 0U ? box.lowPosition.z : box.highPosition.z};
            const auto clipPosition =
                clipMatrix * glm::vec4{cornerPosition, 1.0F};
            const std::array<bool, kClipPlanes> cornerBeyond{
                clipPosition.x < -clipPosition.w,
                clipPosition.x > clipPosition.w,
                clipPosition.y < -clipPosition.w,
                clipPosition.y > clipPosition.w,
                clipPosition.z < -clipPosition.w,
                clipPosition.z > clipPosition.w};

            for (std::size_t plane = 0; plane < kClipPlanes; ++plane)
            {
                allBeyond[plane] = allBeyond[plane] && cornerBeyond[plane];
            }
        }

        return std::ranges::any_of(
            allBeyond, [](const bool beyond) { return beyond; });
    }

}
