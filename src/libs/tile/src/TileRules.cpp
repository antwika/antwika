#include "antwika/tile/TileRules.hpp"

#include <algorithm>
#include <ranges>

namespace antwika::tile
{

    bool TileRules::allows(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile neighbourTile) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        return foundEntry != allowedByEdge.end()
               && foundEntry->second.tiles.contains(neighbourTile);
    }

    void TileRules::set(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile neighbourTile,
        const bool allowed)
    {
        auto &entry = allowedByEdge[Edge{tile, edge}];

        if (allowed)
        {
            entry.tiles.insert(neighbourTile);
        }
        else
        {
            entry.tiles.erase(neighbourTile);
        }

        if (entry.tiles.empty() && !entry.air)
        {
            allowedByEdge.erase(Edge{tile, edge});
        }
    }

    void TileRules::toggle(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile neighbourTile)
    {
        setAllows(
            tile, edge, neighbourTile, !allows(tile, edge, neighbourTile));
    }

    void TileRules::setAllows(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile neighbourTile,
        const bool allowed)
    {
        set(tile, edge, neighbourTile, allowed);
        set(neighbourTile, voxel::getFacing(edge), tile, allowed);
    }

    void TileRules::allow(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Tile neighbourTile)
    {
        allowedByEdge[Edge{tile, edge}].tiles.insert(neighbourTile);
    }

    bool TileRules::hasNoRule(
        const tilemap::Tile tile, const tilemap::TileEdge edge) const
    {
        return !allowedByEdge.contains(Edge{tile, edge});
    }

    bool TileRules::hasNoRuleFor(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge,
        const tilemap::Atlas atlas) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        if (foundEntry == allowedByEdge.end())
        {
            return true;
        }

        if (foundEntry->second.tiles.empty())
        {
            return false;
        }

        return std::ranges::none_of(
            foundEntry->second.tiles,
            [atlas](const tilemap::Tile neighbour)
            { return neighbour.atlas == atlas; });
    }

    bool TileRules::isForbidden(
        const tilemap::Tile tile, const tilemap::TileEdge edge) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        return foundEntry != allowedByEdge.end(
            ) && foundEntry->second.tiles.empty()
               && !foundEntry->second.air;
    }

    bool TileRules::isBoundaryOnly(
        const tilemap::Tile tile, const tilemap::TileEdge edge) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        return foundEntry != allowedByEdge.end(
            ) && foundEntry->second.tiles.empty()
               && foundEntry->second.air;
    }

    bool TileRules::allowsBoundary(
        const tilemap::Tile tile, const tilemap::TileEdge edge) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        return foundEntry == allowedByEdge.end() || foundEntry->second.air;
    }

    void TileRules::setAllowsBoundary(
        const tilemap::Tile tile, const tilemap::TileEdge edge, const bool may)
    {
        auto &entry = allowedByEdge[Edge{tile, edge}];

        entry.air = may;

        if (entry.tiles.empty() && !entry.air)
        {
            allowedByEdge.erase(Edge{tile, edge});
        }
    }

    void TileRules::forbidAll(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge)
    {
        allowedByEdge[Edge{tile, edge}] = AllowedTiles{};
    }

    void TileRules::clearRule(
        const tilemap::Tile tile,
        const tilemap::TileEdge edge)
    {
        allowedByEdge.erase(Edge{tile, edge});
    }

    std::optional<bool> TileRules::getCorner(
        const tilemap::Tile tile, const voxel::Corner corner) const
    {
        const auto foundEntry = byCorner.find({tile, corner});

        return foundEntry == byCorner.end()
                           ? std::optional<bool>{}
                           : std::optional<bool>{foundEntry->second};
    }

    void TileRules::setCorner(
        const tilemap::Tile tile,
        const voxel::Corner corner,
        const std::optional<bool> cornerFilled)
    {
        if (cornerFilled.has_value())
        {
            byCorner[{tile, corner}] = *cornerFilled;

            return;
        }

        byCorner.erase({tile, corner});
    }

    std::vector<std::pair<voxel::Corner, bool>> TileRules::cornersOf(
        const tilemap::Tile tile) const
    {
        std::vector<std::pair<voxel::Corner, bool>> corners;

        for (const auto corner : voxel::kEveryCorner)
        {
            const auto foundEntry = byCorner.find({tile, corner});

            if (foundEntry != byCorner.end())
            {
                corners.emplace_back(corner, foundEntry->second);
            }
        }

        return corners;
    } // GCOVR_EXCL_LINE

    voxel::Kind TileRules::kindOf(const tilemap::Tile tile) const
    {
        const auto foundEntry = byKind.find(tile);

        return foundEntry == byKind.end(
            ) ? voxel::Kind::Normal : foundEntry->second;
    }

    void TileRules::setKind(const tilemap::Tile tile, const voxel::Kind kind)
    {
        if (kind == voxel::Kind::Normal)
        {
            byKind.erase(tile);

            return;
        }

        byKind[tile] = kind;
    }

    std::vector<std::pair<tilemap::Tile, voxel::Kind>> TileRules::getKinds() const
    {
        return {byKind.begin(), byKind.end()};
    } // GCOVR_EXCL_LINE

    voxel::Facing TileRules::facingOf(const tilemap::Tile tile) const
    {
        const auto foundEntry = byFacing.find(tile);

        return foundEntry == byFacing.end(
            ) ? voxel::Facing::Any : foundEntry->second;
    }

    void TileRules::setFacing(
        const tilemap::Tile tile, const voxel::Facing facing)
    {
        if (facing == voxel::Facing::Any)
        {
            byFacing.erase(tile);

            return;
        }

        byFacing[tile] = facing;
    }

    voxel::StairHalf TileRules::levelOf(const tilemap::Tile tile) const
    {
        const auto foundEntry = halfByLevel.find(tile);

        return foundEntry == halfByLevel.end() ? voxel::StairHalf::Any
                           : foundEntry->second;
    }

    void TileRules::setLevel(
        const tilemap::Tile tile, const voxel::StairHalf levelHalf)
    {
        if (levelHalf == voxel::StairHalf::Any)
        {
            halfByLevel.erase(tile);

            return;
        }

        halfByLevel[tile] = levelHalf;
    }

    std::vector<std::pair<tilemap::Tile, voxel::StairHalf>>
    TileRules::getLevels() const
    {
        return {halfByLevel.begin(), halfByLevel.end()};
    } // GCOVR_EXCL_LINE

    voxel::StairPart TileRules::partOf(const tilemap::Tile tile) const
    {
        const auto foundEntry = byPart.find(tile);

        return foundEntry == byPart.end() ? voxel::StairPart::Any
                           : foundEntry->second;
    }

    void TileRules::setPart(
        const tilemap::Tile tile, const voxel::StairPart part)
    {
        if (part == voxel::StairPart::Any)
        {
            byPart.erase(tile);

            return;
        }

        byPart[tile] = part;
    }

    std::vector<std::pair<tilemap::Tile, voxel::StairPart>>
    TileRules::getParts() const
    {
        return {byPart.begin(), byPart.end()};
    } // GCOVR_EXCL_LINE

    std::vector<std::pair<tilemap::Tile, voxel::Facing>>
    TileRules::getFacings() const
    {
        return {byFacing.begin(), byFacing.end()};
    } // GCOVR_EXCL_LINE

    std::vector<TileRule> TileRules::getAllRules() const
    {
        std::vector<TileRule> rules;

        rules.reserve(allowedByEdge.size());

        for (const auto &[edge, tiles] : allowedByEdge)
        {
            rules.push_back(
                TileRule{
                    .tile = edge.first,
                    .edge = edge.second,
                    .allowedTiles = tiles.tiles,
                    .allowsBoundary = tiles.air});
        }

        return rules;
    } // GCOVR_EXCL_LINE

    std::set<tilemap::Tile> TileRules::getAllowed(
        const tilemap::Tile tile, const tilemap::TileEdge edge) const
    {
        const auto foundEntry = allowedByEdge.find(Edge{tile, edge});

        return foundEntry == allowedByEdge.end() ? std::set<tilemap::Tile>{}
                           : foundEntry->second.tiles;
    } // GCOVR_EXCL_LINE

    std::size_t TileRules::getSize() const
    {
        std::size_t count = 0;

        for (const auto &[edge, tiles] : allowedByEdge)
        {
            count += tiles.tiles.size();
        }

        return count;
    }

}
