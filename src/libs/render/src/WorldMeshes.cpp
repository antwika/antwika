#include "antwika/render/WorldMeshes.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

#include <antwika/decor/Decor.hpp>
#include <antwika/decor/Variants.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/assets/MapAssets.hpp>
#include <antwika/tile/Transitions.hpp>

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] bool isSameWeave(
            const std::vector<voxelmap::FaceRef> &oneFaces,
            const std::vector<voxelmap::FaceRef> &otherFaces)
        {
            return std::ranges::equal(
                oneFaces, otherFaces, &voxelmap::FaceRef::isIdenticalTo);
        }
    }

    void WorldMeshes::rebuild(
        gfx::IRenderer &viewportRenderer,
        const map::Map &drawnMap,
        const voxel::Voxels &shownVoxels,
        const solver::CornerSeams joiningSeams,
        const std::array<gfx::Bitmap, 2> &sheetBitmaps,
        const std::uint32_t tick)
    {
        auto wovenRules = tile::getRulesWithTransitions(
            drawnMap.rules,
            drawnMap.transitions,
            sheetBitmaps.at(0),
            sheetBitmaps.at(1),
            drawnMap.paletteColors);
        auto shownFaces = voxelmap::visibleFacesOf(shownVoxels);
        const auto sameWeave = !solvedTiles.empty()
                               && joiningSeams == solvedSeams
                               && wovenRules == effectiveRules
                               && isSameWeave(shownFaces, visibleFaces);

        visibleFaces = std::move(shownFaces);
        effectiveRules = std::move(wovenRules);
        solvedSeams = joiningSeams;

        if (!sameWeave)
        {
            auto woven = assets::faceTilesFor(
                visibleFaces, effectiveRules, joiningSeams, faceTileCache);

            solvedTiles = std::move(woven.tiles);
            weaveSolve = std::move(woven.solve);
        }

        auto appliedTiles = decor::getWithVariantsApplied(
            visibleFaces, solvedTiles, drawnMap.familyGroups, 0);
        const auto sameCells = shownVoxels == meshedVoxels;
        const auto sameLook = sameWeave && sameCells
                              && appliedTiles == finalFaceTiles;

        finalFaceTiles = std::move(appliedTiles);

        const auto meshFor = [this, &viewportRenderer, &shownVoxels](
                                 const voxelmap::Pass pass)
        {
            std::vector<MeshPiece> pieceMeshes;

            for (const auto &piece : voxelmap::getVoxelMeshPieces(
                     shownVoxels, visibleFaces, finalFaceTiles, pass))
            {
                pieceMeshes.push_back(
                    MeshPiece{
                        .mesh = viewportRenderer.createMesh(piece),
                        .box = gfx::getMeshBox(piece)});
            }

            return pieceMeshes;
        };

        if (!sameLook)
        {
            solidMesh = meshFor(voxelmap::Pass::Solid);
            waterMesh = meshFor(voxelmap::Pass::Water);
        }

        if (!sameCells)
        {
            meshedVoxels = shownVoxels;
        }
        decorByFace = decor::getSolveDecorLayers(
            visibleFaces,
            solvedTiles,
            drawnMap.decor,
            drawnMap.decorRules,
            0);
        rebuildDecor(viewportRenderer, drawnMap, tick);
        solidVoxels = drawnMap.voxels;
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

        if (mergedData.vertices.empty())
        {
            decorMesh = nullptr;

            return;
        }

        if (decorMesh
            && decorMesh->getVertexCount() == mergedData.vertices.size()
            && decorMesh->getTriangleCount() == mergedData.getTriangleCount())
        {
            viewportRenderer.updateMesh(*decorMesh, mergedData);

            return;
        }

        decorMesh = viewportRenderer.createMesh(mergedData);
    }

    std::span<const MeshPiece> WorldMeshes::getSolid() const noexcept
    {
        return solidMesh;
    }

    std::span<const MeshPiece> WorldMeshes::getWater() const noexcept
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

    const solver::TileSolve &WorldMeshes::getWeaveSolve() const noexcept
    {
        return weaveSolve;
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
