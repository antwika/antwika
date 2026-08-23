#include "antwika/render/WorldMeshes.hpp"

#include <utility>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/map/MapAssets.hpp>
#include <antwika/tile/Transitions.hpp>

namespace antwika::render
{

    void WorldMeshes::rebuild(
        gfx::IRenderer &viewportRenderer,
        const map::Map &drawnMap,
        voxel::Voxels shownVoxels,
        const solver::CornerSeams joiningSeams,
        const std::array<gfx::Bitmap, 2> &sheetBitmaps,
        const std::uint32_t tick)
    {
        auto viewedMap = drawnMap;

        viewedMap.voxels = shownVoxels;
        effectiveRules = tile::getRulesWithTransitions(
            drawnMap.rules,
            drawnMap.transitions,
            sheetBitmaps.at(0),
            sheetBitmaps.at(1),
            drawnMap.paletteColors);
        viewedMap.rules = effectiveRules;
        solvedTiles =
            map::faceTilesFor(viewedMap, joiningSeams, faceTileCache);
        visibleFaces = voxelmap::visibleFacesOf(shownVoxels);
        finalFaceTiles = decor::getWithVariantsApplied(
            visibleFaces, solvedTiles, drawnMap.familyGroups, 0);

        const auto meshFor = [this, &viewportRenderer, &shownVoxels](
                                 const voxelmap::Pass pass)
        {
            const auto mesh = voxelmap::getVoxelMesh(
                shownVoxels, visibleFaces, finalFaceTiles, pass);

            std::vector<std::unique_ptr<gfx::IMesh>> pieceMeshes;

            if (mesh.vertices.empty())
            {
                return pieceMeshes;
            }

            for (const auto &piece :
                 gfx::getSplitMesh(mesh, voxelmap::kMeshPieceVertices))
            {
                pieceMeshes.push_back(viewportRenderer.createMesh(piece));
            }

            return pieceMeshes;
        };

        solidMesh = meshFor(voxelmap::Pass::Solid);
        waterMesh = meshFor(voxelmap::Pass::Water);
        decorByFace = decor::getSolveDecorLayers(
            visibleFaces,
            solvedTiles,
            drawnMap.decor,
            drawnMap.decorRules,
            0);
        rebuildDecor(viewportRenderer, drawnMap, tick);
        solidVoxels = voxel::Voxels(
            drawnMap.voxels.begin(), drawnMap.voxels.end());
    }

    void WorldMeshes::rebuildDecor(
        gfx::IRenderer &viewportRenderer,
        const map::Map &drawnMap,
        const std::uint32_t tick)
    {
        gfx::MeshData mergedData;

        for (std::size_t rank = 0; rank < decorByFace.size(); ++rank)
        {
            const auto mesh = decor::getDecorMesh(
                visibleFaces,
                decorByFace[rank].second,
                drawnMap.decor,
                tick,
                decor::kDecorDepthBias
                    * static_cast<float>(rank + 1));
            const auto first =
                static_cast<std::uint32_t>(mergedData.vertices.size());

            mergedData.vertices.insert(
                mergedData.vertices.end(),
                mesh.vertices.begin(),
                mesh.vertices.end());

            for (const auto index : mesh.indices)
            {
                mergedData.indices.push_back(first + index);
            }
        }

        decorMesh = mergedData.vertices.empty()
                  ? nullptr
                  : viewportRenderer.createMesh(mergedData);
    }

    std::span<const std::unique_ptr<gfx::IMesh>> WorldMeshes::getSolid()
        const noexcept
    {
        return solidMesh;
    }

    std::span<const std::unique_ptr<gfx::IMesh>> WorldMeshes::getWater()
        const noexcept
    {
        return waterMesh;
    }

    const gfx::IMesh *WorldMeshes::getDecor() const noexcept
    {
        return decorMesh.get();
    }

    const voxel::Voxels &WorldMeshes::getCells()
        const noexcept
    {
        return solidVoxels;
    }

    const std::vector<voxelmap::FaceRef> &WorldMeshes::getFaces()
        const noexcept
    {
        return visibleFaces;
    }

    const std::vector<tilemap::Tile> &WorldMeshes::getSolvedTiles()
        const noexcept
    {
        return solvedTiles;
    }

    const std::vector<tilemap::Tile> &WorldMeshes::getDrawnAs()
        const noexcept
    {
        return finalFaceTiles;
    }

    const tile::TileRules &WorldMeshes::getRules() const noexcept
    {
        return effectiveRules;
    }

    const std::vector<
        std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>> &
    WorldMeshes::getDecorLayers() const noexcept
    {
        return decorByFace;
    }

}
