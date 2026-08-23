#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TileRules.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::render
{

    class WorldMeshes final
    {
    public:
        void rebuild(
            gfx::IRenderer &viewportRenderer,
            const map::Map &drawnMap,
            voxel::Voxels shownVoxels,
            solver::CornerSeams joiningSeams,
            const std::array<gfx::Bitmap, 2> &sheetBitmaps,
            std::uint32_t tick);

        void rebuildDecor(
            gfx::IRenderer &viewportRenderer,
            const map::Map &drawnMap,
            std::uint32_t tick);

        [[nodiscard]] std::span<const std::unique_ptr<gfx::IMesh>>
        getSolid() const noexcept;

        [[nodiscard]] std::span<const std::unique_ptr<gfx::IMesh>>
        getWater() const noexcept;

        [[nodiscard]] const gfx::IMesh *getDecor() const noexcept;

        [[nodiscard]] const voxel::Voxels &getCells()
            const noexcept;

        [[nodiscard]] const std::vector<voxelmap::FaceRef> &getFaces()
            const noexcept;

        [[nodiscard]] const std::vector<tilemap::Tile> &getSolved()
            const noexcept;

        [[nodiscard]] const std::vector<tilemap::Tile> &getDrawnAs()
            const noexcept;

        [[nodiscard]] const tile::TileRules &getRules() const noexcept;

        [[nodiscard]] const std::vector<
            std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
            &getDecorLayers() const noexcept;

    private:
        std::map<voxelmap::FaceRef, tilemap::Tile> faceTileCache;
        std::vector<tilemap::Tile> solvedTiles;
        std::vector<tilemap::Tile> finalFaceTiles;
        std::vector<voxelmap::FaceRef> visibleFaces;
        tile::TileRules effectiveRules;
        std::vector<
            std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
            decorByFace;
        std::vector<std::unique_ptr<gfx::IMesh>> solidMesh;
        std::vector<std::unique_ptr<gfx::IMesh>> waterMesh;
        std::unique_ptr<gfx::IMesh> decorMesh;
        voxel::Voxels solidVoxels;
    };

}
