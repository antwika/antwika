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
#include <antwika/gfx/ViewportRenderer.hpp>
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
            gfx::ViewportRenderer &viewportRenderer,
            const map::Map &drawnMap,
            voxel::Voxels shownVoxels,
            solver::CornerSeams joiningSeams,
            const std::array<gfx::Bitmap, 2> &sheetBitmaps,
            std::uint32_t tick);

        void rebuildDecor(
            gfx::ViewportRenderer &viewportRenderer,
            const map::Map &drawnMap,
            std::uint32_t tick);

        [[nodiscard]] std::span<const std::unique_ptr<gfx::IMesh>>
        solid() const noexcept;

        [[nodiscard]] std::span<const std::unique_ptr<gfx::IMesh>>
        water() const noexcept;

        [[nodiscard]] const gfx::IMesh *decor() const noexcept;

        [[nodiscard]] const voxel::Voxels &cells()
            const noexcept;

        [[nodiscard]] const std::vector<voxelmap::FaceRef> &faces()
            const noexcept;

        [[nodiscard]] const std::vector<tilemap::Tile> &solved()
            const noexcept;

        [[nodiscard]] const std::vector<tilemap::Tile> &drawnAs()
            const noexcept;

        [[nodiscard]] const tile::TileRules &rules() const noexcept;

        [[nodiscard]] const std::vector<
            std::pair<std::size_t, std::map<std::size_t, tilemap::Tile>>>
            &decorLayers() const noexcept;

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
