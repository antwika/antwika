#include "antwika/decor/Variants.hpp"

#include <algorithm>
#include <cstddef>

#include "DecorDetail.hpp"

namespace antwika::decor
{

    namespace
    {
        using decordetail::hashMix;

        [[nodiscard]] std::uint32_t rolledAt(
            const voxelmap::FaceRef &face, const std::uint32_t seed)
        {
            auto mixedSeed = static_cast<std::uint32_t>(face.cell.position().x)
                         * 73856093U;

            mixedSeed ^= static_cast<std::uint32_t>(face.cell.position().y)
                     * 19349663U;
            mixedSeed ^= static_cast<std::uint32_t>(face.cell.position().z)
                     * 83492791U;
            mixedSeed ^= (static_cast<std::uint32_t>(face.side) + 1U)
                     * 2654435761U;
            mixedSeed ^= seed * 2246822519U;

            return hashMix(mixedSeed | 1U);
        }

        [[nodiscard]] std::uint32_t weightOf(
            const VariantGroup &familyGroup)
        {
            std::uint32_t total = familyGroup.weight;

            for (const auto &member : familyGroup.variants)
            {
                total += member.weight;
            }

            return total;
        }

        [[nodiscard]] bool hasRules(
            const tile::TileRules &rules, const tilemap::Tile tile)
        {
            for (const auto edge : tilemap::kEveryTileEdge)
            {
                if (!rules.hasNoRuleFor(
                        tile, edge, tilemap::Atlas::Wall)
                    || !rules.hasNoRuleFor(
                        tile, edge, tilemap::Atlas::Floor))
                {
                    return true;
                }
            }

            return false;
        }
    }

    const VariantGroup *groupLedBy(
        const std::span<const VariantGroup> familyGroups,
        const tilemap::Tile tile)
    {
        for (const auto &family : familyGroups)
        {
            if (family.canonicalTile == tile)
            {
                return &family;
            }
        }

        return nullptr;
    }

    const VariantGroup *groupContaining(
        const std::span<const VariantGroup> familyGroups,
        const tilemap::Tile tile)
    {
        for (const auto &family : familyGroups)
        {
            for (const auto &member : family.variants)
            {
                if (member.tile == tile)
                {
                    return &family;
                }
            }
        }

        return nullptr;
    }

    tilemap::Tile canonicalTileOf(
        const std::span<const VariantGroup> familyGroups,
        const tilemap::Tile tile)
    {
        const auto *group = groupContaining(familyGroups, tile);

        return group == nullptr ? tile : group->canonicalTile;
    }

    std::vector<VariantGroup> withVariantToggled(
        const std::vector<VariantGroup> &familyGroups,
        const tilemap::Tile canonicalTile,
        const tilemap::Tile tile)
    {
        if (canonicalTile == tile)
        {
            return familyGroups;
        }

        auto updatedGroups = familyGroups;

        for (std::size_t index = 0; index < updatedGroups.size(); ++index)
        {
            auto &family = updatedGroups.at(index);

            if (family.canonicalTile != canonicalTile)
            {
                continue;
            }

            const auto erasedCount = std::erase_if(
                family.variants,
                [tile](const VariantMember &member)
                { return member.tile == tile; });

            if (erasedCount == 0)
            {
                family.variants.push_back(
                    VariantMember{.tile = tile});
            }
            else if (family.variants.empty())
            {
                updatedGroups.erase(
                    std::next(
                        updatedGroups.begin(),
                        static_cast<std::ptrdiff_t>(index)));
            }

            return updatedGroups;
        }

        updatedGroups.push_back(
            VariantGroup{
                .canonicalTile = canonicalTile,
                .variants = {VariantMember{.tile = tile}}});

        return updatedGroups;
    } // GCOVR_EXCL_LINE

    std::vector<VariantGroup> withVariantWeightSet(
        const std::vector<VariantGroup> &familyGroups,
        const tilemap::Tile tile,
        const std::uint8_t weight)
    {
        const auto clampedWeight = std::min(weight, kFullFrequency);
        auto updatedGroups = familyGroups;

        for (auto &family : updatedGroups)
        {
            if (family.canonicalTile == tile)
            {
                family.weight = clampedWeight;

                return updatedGroups;
            }

            for (auto &member : family.variants)
            {
                if (member.tile == tile)
                {
                    member.weight = clampedWeight;

                    return updatedGroups;
                }
            }
        }

        return updatedGroups;
    } // GCOVR_EXCL_LINE

    bool canBeVariantOf(
        const std::span<const VariantGroup> familyGroups,
        const tile::TileRules &rules,
        const std::span<const DecorTile> decor,
        const tilemap::Tile canonicalTile,
        const tilemap::Tile tile)
    {
        if (canonicalTile == tile || canonicalTile.atlas != tile.atlas
            || groupContaining(familyGroups, canonicalTile) != nullptr
            || groupLedBy(familyGroups, tile) != nullptr
            || decorOf(decor, tile) != nullptr)
        {
            return false;
        }

        const auto *group = groupContaining(familyGroups, tile);

        if (group != nullptr && group->canonicalTile != canonicalTile)
        {
            return false;
        }

        return !hasRules(rules, tile)
               && rules.cornersOf(tile).empty()
               && rules.kindOf(tile) == voxel::Kind::Normal
               && rules.facingOf(tile) == voxel::Facing::Any
               && rules.levelOf(tile) == voxel::StairHalf::Any;
    }

    std::vector<tilemap::Tile> withVariantsApplied(
        const std::vector<voxelmap::FaceRef> &faces,
        const std::span<const tilemap::Tile> wovenTiles,
        const std::span<const VariantGroup> familyGroups,
        const std::uint32_t seed)
    {
        std::vector<tilemap::Tile> tiles(wovenTiles.begin(), wovenTiles.end());

        if (familyGroups.empty())
        {
            return tiles;
        }

        for (std::size_t index = 0;
             index < faces.size() && index < tiles.size();
             ++index)
        {
            const auto *family = groupLedBy(familyGroups, tiles.at(index));

            if (family == nullptr)
            {
                continue;
            }

            const auto total = weightOf(*family);

            if (total == 0)
            {
                continue;
            }

            const auto rollValue =
                rolledAt(faces.at(index), seed) % total;
            std::uint32_t runningWeight = family->weight;

            if (rollValue < runningWeight)
            {
                continue;
            }

            for (const auto &member : family->variants)
            {
                runningWeight += member.weight;

                if (rollValue < runningWeight)
                {
                    tiles.at(index) = member.tile;

                    break;
                }
            }
        }

        return tiles;
    } // GCOVR_EXCL_LINE

}
