#include <utility>
#include <vector>

#include <antwika/component/Position.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>
#include <antwika/pathfinding/Path.hpp>
#include <antwika/collision/VoxelWalkGraph.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/collision/VoxelWalkGraph.hpp"

namespace
{

    constexpr std::uint64_t kMaxGuideSteps = 20000;

}

namespace antwika::editor
{

    void Editor::pathTo(const input::Position position)
    {
        const auto projectToScreen = viewportRenderer.viewport().toCanvas(
            antwika::gfx::Point{.x = position.x, .y = position.y});
        const antwika::gfx::PointF point{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};
        const auto ray = voxelmap::rayInModelSpace(
            voxelmap::rayThrough(
                worldCamera(),
                camera::kCanvasSize,
                point),
            worldRotation());
        const auto hit = voxelmap::raycastFace(map.voxels, ray);

        if (!hit.has_value())
        {
            return;
        }

        const auto standing =
            game->world().get<component::Position>(game->player());
        const auto fromStood = collision::supportingVoxel(
            worldMeshes.cells(),
            static_cast<std::int32_t>(
                std::floor(standing.x / antwika::voxel::kVoxelSide)),
            static_cast<std::int32_t>(
                std::floor(standing.z / antwika::voxel::kVoxelSide)),
            standing.y);
        const auto landing =
            collision::restPositionOverColumn(worldMeshes.cells(), hit->cell.x,
                hit->cell.z);

        if (!fromStood.has_value() || !landing.has_value())
        {
            return;
        }

        const auto toStood = collision::supportingVoxel(
            worldMeshes.cells(), hit->cell.x, hit->cell.z, landing->y);

        if (!toStood.has_value())
        {
            return;
        }

        const collision::VoxelWalkGraph walkGraph(worldMeshes.cells());
        const auto walk = antwika::pathfinding::pathBetween(
            walkGraph,
            antwika::pathfinding::GridPos{
                .x = fromStood->x,
                .y = fromStood->y,
                .z = fromStood->z},
            antwika::pathfinding::GridPos{
                .x = toStood->x,
                .y = toStood->y,
                .z = toStood->z},
            kMaxGuideSteps);

        if (!walk.has_value())
        {
            return;
        }

        std::vector<antwika::gfx::Vec3> stopPositions;

        for (const auto &stop : *walk)
        {
            stopPositions.push_back(
                antwika::gfx::Vec3{
                    static_cast<float>(stop.x),
                    0.0F,
                    static_cast<float>(stop.z)});
        }

        game->followPath(std::move(stopPositions), *toStood);
    }

}
