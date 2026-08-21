#include "antwika/decor/Decor.hpp"

#include <algorithm>
#include <array>

#include <antwika/tilemap/AtlasLayout.hpp>
#include <antwika/gfx/MeshData.hpp>

namespace antwika::decor
{

    namespace
    {
        constexpr std::uint64_t kFirstFrameWidget = 225;
    }

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

    std::vector<DecorTile> withDecorToggled(
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

    std::vector<DecorTile> withBaseToggled(
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

    std::vector<DecorTile> withDecorLayerSet(
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

    std::vector<DecorTile> withWeightSet(
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

    std::vector<DecorTile> withFrequencySet(
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

    bool decorSpanned(const DecorTile &decor)
    {
        return decor.width > 1 || decor.height > 1;
    }

    std::vector<DecorTile> withSpanSet(
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

            if (decorSpanned(record))
            {
                record.frameTiles = {tile};
            }
        }

        return updatedDecor;
    } // GCOVR_EXCL_LINE

    std::vector<DecorTile> withMemberSet(
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

    std::vector<DecorTile> compactedDecor(
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

    std::vector<DecorTile> withFrameAdded(
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

    std::vector<DecorTile> withFrameSet(
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

    gfx::MeshData decorMesh(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::map<std::size_t, tilemap::Tile> &placedTiles,
        const std::span<const DecorTile> decor,
        const time::Tick tick,
        const float lift)
    {
        constexpr std::array<std::pair<float, float>, 4> kWithin{
            std::pair{0.0F, 1.0F},
            std::pair{1.0F, 1.0F},
            std::pair{1.0F, 0.0F},
            std::pair{0.0F, 0.0F}};

        gfx::MeshData mesh;

        for (const auto &[faceIndex, identity] : placedTiles)
        {
            const auto *record = decorOf(decor, identity);
            const auto frame = record != nullptr
                             ? decorFrameAt(*record, tick)
                             : identity;
            const auto tile = tilemap::tileCoords(
                frame.index, tilemap::tileSizeOf(frame.atlas));
            const auto &face = faces[faceIndex];
            const auto middlePoint = voxelmap::cellMiddle(face.cell);
            const auto liftedPoint =
                gfx::Vec3(voxelmap::faceNormal(face.side)) * lift;
            const auto climbs =
                face.climbCell.x != 0 || face.climbCell.z != 0;
            const auto flight = climbs
                              ? voxel::stairQuads(face.climbCell)
                              : std::vector<voxel::StairQuad>{};

            std::vector<voxel::StairQuad> layingQuads;

            for (const auto &quad : flight)
            {
                if (quad.side == face.side)
                {
                    layingQuads.push_back(quad);
                }
            }

            if (!climbs)
            {
                voxel::StairQuad wholeQuad{.side = face.side};

                for (std::size_t corner = 0;
                     corner < wholeQuad.corners.size();
                     ++corner)
                {
                    wholeQuad.corners.at(corner) = gfx::Vec3(
                        voxelmap::faceCorner(face.side, corner));
                }

                layingQuads.push_back(wholeQuad);
            }

            for (const auto &quad : layingQuads)
            {
                const auto part = voxelmap::stairUvRect(tile, quad);
                const auto first = static_cast<std::uint32_t>(
                    mesh.vertices.size());

                for (std::size_t corner = 0;
                     corner < kWithin.size();
                     ++corner)
                {
                    mesh.vertices.push_back(
                        gfx::Vertex3D{
                            .position =
                                middlePoint + quad.corners[corner]
                                + liftedPoint,
                            .normal = voxelmap::faceNormal(face.side),
                            .texCoordinate =
                                gfx::Vec2{
                                    part.originPoint.x
                                        + (kWithin[corner]
                                               .first
                                           * part.size
                                                 .width),
                                    part.originPoint.y
                                        + (kWithin[corner]
                                               .second
                                           * part.size
                                                 .height)}});
                }

                for (const std::uint32_t step :
                     {0U, 1U, 2U, 0U, 2U, 3U})
                {
                    mesh.indices.push_back(first + step);
                }
            }
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

    ui::WidgetId frameWidget(const std::size_t frame)
    {
        return ui::WidgetId{
            kFirstFrameWidget + static_cast<std::uint64_t>(frame)};
    }

    ui::WidgetId memberWidget(const std::size_t member)
    {
        return ui::WidgetId{
            386 + static_cast<std::uint64_t>(member)};
    }

}
