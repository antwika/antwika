#include "antwika/decor/Decor.hpp"

#include <algorithm>
#include <array>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/voxelmap/QuadPaint.hpp>

namespace antwika::decor
{

    const DecorTile *decorOf(
        const std::span<const DecorTile> decor, const tilemap::Tile tile)
    {
        for (const auto &record : decor)
        {
            if (record.tile == tile)
            {
                return &record;
            }
        }

        return nullptr;
    }

    std::vector<DecorTile> getWithDecorToggled(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::size_t layer)
    {
        auto updatedDecor = decor;
        const auto foundDecor = std::find_if(
            updatedDecor.begin(),
            updatedDecor.end(),
            [tile](const DecorTile &one) { return one.tile == tile; });

        if (foundDecor != updatedDecor.end())
        {
            updatedDecor.erase(foundDecor);

            return updatedDecor;
        }

        updatedDecor.push_back(
            DecorTile{
                .tile = tile,
                .frameTiles = {tile},
                .allowedBaseTiles = {},
                .layer = layer,
                .spanTiles = {tile}});

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithBaseToggled(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const tilemap::Tile baseTile)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile != tile)
            {
                continue;
            }

            const auto foundTile = std::find(
                record.allowedBaseTiles.begin(),
                record.allowedBaseTiles.end(),
                baseTile);

            if (foundTile != record.allowedBaseTiles.end())
            {
                record.allowedBaseTiles.erase(foundTile);
            }
            else
            {
                record.allowedBaseTiles.push_back(baseTile);
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithDecorLayerSet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::size_t layer)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile)
            {
                record.layer = layer;
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithWeightSet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::uint8_t weight)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile)
            {
                record.weight =
                    std::min(weight, kFullFrequency);
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithFrequencySet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::uint8_t frequency)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile)
            {
                record.frequency =
                    std::min(frequency, kFullFrequency);
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    bool isDecorSpanned(const DecorTile &decor)
    {
        return decor.width > 1 || decor.height > 1;
    }

    std::vector<DecorTile> getWithSpanSet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::uint8_t acrossSpan,
        const std::uint8_t downSpan)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile != tile)
            {
                continue;
            }

            const auto spanWidth = std::clamp<std::uint8_t>(
                acrossSpan, 1, kMaxDecorSpan);
            const auto spanHeight = std::clamp<std::uint8_t>(
                downSpan, 1, kMaxDecorSpan);
            std::vector<tilemap::Tile> grownTiles;

            for (std::uint8_t row = 0; row < spanHeight; ++row)
            {
                for (std::uint8_t column = 0;
                     column < spanWidth;
                     ++column)
                {
                    const auto keptTile =
                        row < record.height
                        && column < record.width;
                    const auto place =
                        (static_cast<std::size_t>(row)
                         * record.width)
                        + column;

                    grownTiles.push_back(
                        keptTile && place < record.spanTiles.size()
                            ? record.spanTiles.at(place)
                            : tile);
                }
            }

            grownTiles.at(0) = tile;
            record.width = spanWidth;
            record.height = spanHeight;
            record.spanTiles = grownTiles;

            if (isDecorSpanned(record))
            {
                record.frameTiles = {tile};
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithMemberSet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::size_t member,
        const tilemap::Tile drawnTile)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile && member > 0
                && member < record.spanTiles.size()
                && drawnTile.atlas == tile.atlas)
            {
                record.spanTiles.at(member) = drawnTile;
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getCompactedDecor(
        const std::vector<DecorTile> &decor)
    {
        auto updatedDecor = decor;

        std::erase_if(
            updatedDecor,
            [](const DecorTile &record)
            {
                return record
                       == DecorTile{
                           .tile = record.tile,
                           .frameTiles = {record.tile},
                           .spanTiles = {record.tile}};
            });

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithFrameAdded(
        const std::vector<DecorTile> &decor, const tilemap::Tile tile)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile
                && record.frameTiles.size() < kMaxDecorFrames)
            {
                record.frameTiles.push_back(
                    record.frameTiles.empty()
                        ? record.tile
                        : record.frameTiles.back());
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> getWithFrameSet(
        const std::vector<DecorTile> &decor,
        const tilemap::Tile tile,
        const std::size_t frame,
        const tilemap::Tile drawnTile)
    {
        auto updatedDecor = decor;

        for (auto &record : updatedDecor)
        {
            if (record.tile == tile && frame > 0
                && frame < record.frameTiles.size())
            {
                record.frameTiles[frame] = drawnTile;
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    gfx::MeshData getDecorMesh(
        const voxel::Voxels &voxels,
        const std::vector<voxelmap::FaceRef> &faces,
        const std::map<std::size_t, tilemap::Tile> &placedTiles,
        const std::span<const DecorTile> decor,
        const time::Tick tick,
        const float lift)
    {
        gfx::MeshData mesh;

        for (const auto &[faceIndex, identity] : placedTiles)
        {
            const auto *record = decorOf(decor, identity);
            const auto frame = record != nullptr
                             ? decorFrameAt(*record, tick)
                             : identity;
            const auto tile = tilemap::getTileCoords(
                frame.index, tilemap::tileSizeOf(frame.atlas));
            const auto &face = faces[faceIndex];
            const auto climbs =
                face.climbPosition.x != 0 || face.climbPosition.z != 0;

            // The overlay bevels with the face beneath it, so decor
            // is never left hanging over a sunken border band.
            voxelmap::addFaceQuads(
                mesh,
                voxels,
                face.side,
                face.climbPosition,
                climbs,
                voxelmap::getCellMiddle(face.cell.position),
                voxelmap::QuadPaint{
                    .tileRect = tile,
                    .liftPoint =
                        gfx::Vec3(voxelmap::getFaceNormal(face.side))
                        * lift,
                    .beveled = face.cell.material.kind
                               == voxel::Kind::Normal});
        }

        return mesh;
    } // GCOVR_EXCL_LINE

    tilemap::Tile decorFrameAt(const DecorTile &decor, const time::Tick tick)
    {
        if (decor.frameTiles.empty())
        {
            return decor.tile;
        }

        return decor.frameTiles[static_cast<std::size_t>(tick / kDecorPaceTick)
                            % decor.frameTiles.size()];
    }

    bool hasAnimatedDecor(const std::span<const DecorTile> decor)
    {
        return std::any_of(
            decor.begin(),
            decor.end(),
            [](const DecorTile &record)
            { return record.frameTiles.size() > 1; });
    }

    bool tilesCompatible(
        const tile::TileRules &rules,
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile otherTile)
    {
        return rules.hasNoRuleFor(tile, edge, otherTile.atlas)
               || rules.allows(tile, edge, otherTile);
    }

}
