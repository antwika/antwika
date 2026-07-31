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

        /// Half the cube's edge, so it spans one unit in each axis.
        constexpr float kHalf = 0.5F;

        /// How many corners one face has.
        constexpr std::size_t kCorners = 4;

        /**
         * @brief One side of the cube.
         */
        struct Face
        {
            Vec3 normal;
            std::array<Vec3, kCorners> corners;
            Color color;
        };

        /**
         * @brief The six faces, each wound anticlockwise from outside.
         *
         * Listed corner by corner rather than derived from the normal,
         * because deriving a winding is exactly the step that gets a
         * cube turned inside out without anything failing.
         */
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

        /// Where each corner samples a texture, in the same order.
        constexpr std::array<Vec2, kCorners> kCornerTexCoords{
            Vec2{0.0F, 0.0F},
            Vec2{1.0F, 0.0F},
            Vec2{1.0F, 1.0F},
            Vec2{0.0F, 1.0F}};

    } // namespace

    MeshData cubeMesh()
    {
        MeshData mesh;

        mesh.vertices.reserve(kFaces.size() * kCorners);
        mesh.indices.reserve(kFaces.size() * 6);

        for (const Face &face : kFaces)
        {
            const auto first =
                static_cast<std::uint32_t>(mesh.vertices.size());

            for (std::size_t corner = 0; corner < kCorners; ++corner)
            {
                // Indexed rather than at().
                // The loop bound is the array bound.
                // So at()'s check is a branch no test could take.
                mesh.vertices.push_back(
                    Vertex3D{
                        .position = face.corners[corner],
                        .normal = face.normal,
                        .texCoord = kCornerTexCoords[corner],
                        .color = face.color});
            }

            mesh.indices.insert(
                mesh.indices.end(),
                {first,
                 first + 1,
                 first + 2,
                 first,
                 first + 2,
                 first + 3});
        }

        return mesh;
    }

} // namespace antwika::gfx3d_demo
