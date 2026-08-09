#include "antwika/gfx3d_demo/CubeMesh.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>

namespace antwika::gfx3d_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Vec2;
    using antwika::gfx::Vec3;
    using antwika::gfx::Vertex3D;

    namespace
    {

        constexpr float kHalf = 0.5F;

        constexpr std::size_t kCorners = 4;

        struct Face final
        {
            Vec3 normal;
            std::array<Vec3, kCorners> corners;
            Color color;
        };

        constexpr std::array<Face, 6> kFaces{
            Face{
                .normal = {0.0F, 0.0F, 1.0F},
                .corners =
                    {Vec3{-kHalf, -kHalf, kHalf},
                     Vec3{kHalf, -kHalf, kHalf},
                     Vec3{kHalf, kHalf, kHalf},
                     Vec3{-kHalf, kHalf, kHalf}},
                .color = {.red = 220, .green = 60, .blue = 60}},
            Face{
                .normal = {0.0F, 0.0F, -1.0F},
                .corners =
                    {Vec3{kHalf, -kHalf, -kHalf},
                     Vec3{-kHalf, -kHalf, -kHalf},
                     Vec3{-kHalf, kHalf, -kHalf},
                     Vec3{kHalf, kHalf, -kHalf}},
                .color = {.red = 60, .green = 200, .blue = 90}},
            Face{
                .normal = {1.0F, 0.0F, 0.0F},
                .corners =
                    {Vec3{kHalf, -kHalf, kHalf},
                     Vec3{kHalf, -kHalf, -kHalf},
                     Vec3{kHalf, kHalf, -kHalf},
                     Vec3{kHalf, kHalf, kHalf}},
                .color = {.red = 70, .green = 120, .blue = 230}},
            Face{
                .normal = {-1.0F, 0.0F, 0.0F},
                .corners =
                    {Vec3{-kHalf, -kHalf, -kHalf},
                     Vec3{-kHalf, -kHalf, kHalf},
                     Vec3{-kHalf, kHalf, kHalf},
                     Vec3{-kHalf, kHalf, -kHalf}},
                .color = {.red = 230, .green = 200, .blue = 60}},
            Face{
                .normal = {0.0F, 1.0F, 0.0F},
                .corners =
                    {Vec3{-kHalf, kHalf, kHalf},
                     Vec3{kHalf, kHalf, kHalf},
                     Vec3{kHalf, kHalf, -kHalf},
                     Vec3{-kHalf, kHalf, -kHalf}},
                .color = {.red = 200, .green = 80, .blue = 200}},
            Face{
                .normal = {0.0F, -1.0F, 0.0F},
                .corners =
                    {Vec3{-kHalf, -kHalf, -kHalf},
                     Vec3{kHalf, -kHalf, -kHalf},
                     Vec3{kHalf, -kHalf, kHalf},
                     Vec3{-kHalf, -kHalf, kHalf}},
                .color = {.red = 60, .green = 200, .blue = 210}}};

        constexpr std::array<std::uint32_t, 6> kQuadIndices{
            0, 1, 2, 0, 2, 3};

        constexpr std::array<Vec2, kCorners> kCornerTexCoords{
            Vec2{0.0F, 0.0F},
            Vec2{1.0F, 0.0F},
            Vec2{1.0F, 1.0F},
            Vec2{0.0F, 1.0F}};

    }

    MeshData cubeMesh()
    {
        MeshData mesh;

        mesh.vertices.reserve(kFaces.size() * kCorners);
        mesh.indices.reserve(kFaces.size() * kQuadIndices.size());

        for (const Face &face : kFaces)
        {
            const auto first =
                static_cast<std::uint32_t>(mesh.vertices.size());

            for (std::size_t corner = 0; corner < kCorners; ++corner)
            {
                mesh.vertices.push_back(
                    Vertex3D{
                        .position = face.corners[corner],
                        .normal = face.normal,
                        .texCoord = kCornerTexCoords[corner],
                        .color = face.color});
            }

            for (const std::uint32_t offset : kQuadIndices)
            {
                mesh.indices.push_back(first + offset);
            }
        }

        return mesh;
    } // GCOVR_EXCL_LINE

}
