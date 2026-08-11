#include "antwika/map_editor/TilesetPreview.hpp"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <optional>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tileset/Sprite.hpp>

namespace antwika::map_editor
{

    namespace
    {
        using antwika::tileset::kEdgeSocket;
        using antwika::tileset::kOpenSocket;
        using antwika::tileset::Side;
        using antwika::tileset::SocketId;
        using antwika::tileset::Sprite;

        constexpr auto kNorth = enums::index(Side::North);
        constexpr auto kEast = enums::index(Side::East);
        constexpr auto kSouth = enums::index(Side::South);
        constexpr auto kWest = enums::index(Side::West);

        constexpr std::array<std::size_t, 4> kOpposite{
            kSouth, kWest, kNorth, kEast};

        constexpr std::array<std::int32_t, 4> kDeltaColumn{
            0, 1, 0, -1};

        constexpr std::array<std::int32_t, 4> kDeltaRow{
            -1, 0, 1, 0};

        struct NeighborWants final
        {
            std::array<std::optional<SocketId>, 4> socket{};

            [[nodiscard]] std::int32_t count() const noexcept
            {
                std::int32_t held = 0;

                for (const auto &want : socket)
                {
                    held += want.has_value() ? 1 : 0;
                }

                return held;
            }
        };

        [[nodiscard]] constexpr std::size_t cellIndex(
            const std::int32_t column,
            const std::int32_t row) noexcept
        {
            return static_cast<std::size_t>(row)
                       * static_cast<std::size_t>(kPreviewColumns)
                   + static_cast<std::size_t>(column);
        }

        [[nodiscard]] constexpr bool inLattice(
            const std::int32_t column,
            const std::int32_t row) noexcept
        {
            return column >= 0 && row >= 0
                   && column < kPreviewColumns && row < kPreviewRows;
        }

        [[nodiscard]] constexpr bool regionOutside(
            const std::int32_t column,
            const std::int32_t row,
            const std::array<bool, 4> &edges) noexcept
        {
            return (edges[kNorth] && row < kPreviewCenterRow)
                   || (edges[kSouth] && row > kPreviewCenterRow)
                   || (edges[kWest] && column < kPreviewCenterColumn)
                   || (edges[kEast]
                       && column > kPreviewCenterColumn);
        }

        [[nodiscard]] std::uint32_t nextRandom(
            std::uint32_t &state) noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;

            return state;
        }

        [[nodiscard]] std::uint64_t cellHash(
            const std::int32_t column,
            const std::int32_t row,
            const std::uint32_t seed) noexcept
        {
            constexpr std::uint64_t kColumnMix =
                0x9E3779B97F4A7C15ULL;
            constexpr std::uint64_t kRowMix = 0xBF58476D1CE4E5B9ULL;
            constexpr std::uint64_t kFinalMix = 0x94D049BB133111EBULL;

            auto mixed =
                static_cast<std::uint64_t>(column) * kColumnMix
                ^ static_cast<std::uint64_t>(row) * kRowMix
                ^ (static_cast<std::uint64_t>(seed) + 1) * kFinalMix;

            mixed ^= mixed >> 31;
            mixed *= kFinalMix;
            mixed ^= mixed >> 29;

            return mixed;
        }

        [[nodiscard]] std::size_t weightedPick(
            const std::vector<Sprite> &sprites,
            const std::vector<std::size_t> &candidates,
            const std::uint64_t roll)
        {
            std::uint64_t total = 0;

            for (const auto candidate : candidates)
            {
                total += sprites[candidate].weight;
            }

            auto left = roll % total;
            std::size_t at = 0;

            while (left >= sprites[candidates[at]].weight)
            {
                left -= sprites[candidates[at]].weight;
                ++at;
            }

            return candidates[at];
        }

        [[nodiscard]] bool shapeFits(
            const Sprite &sprite,
            const std::array<bool, 4> &out) noexcept
        {
            for (std::size_t side = 0; side < 4; ++side)
            {
                if (out[side]
                    != (sprite.sockets[side] == kEdgeSocket))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::int32_t matchCount(
            const Sprite &sprite,
            const NeighborWants &wants) noexcept
        {
            std::int32_t matched = 0;

            for (std::size_t side = 0; side < 4; ++side)
            {
                if (wants.socket[side].has_value()
                    && sprite.sockets[side] == *wants.socket[side])
                {
                    ++matched;
                }
            }

            return matched;
        }

        [[nodiscard]] std::size_t chooseBase(
            const std::vector<Sprite> &sprites,
            const std::array<bool, 4> &out,
            const NeighborWants &wants,
            std::uint32_t &rng)
        {
            std::vector<std::size_t> candidates{};

            for (bool needShape = true; candidates.empty();
                 needShape = false)
            {
                for (auto required = wants.count();
                     required >= 0 && candidates.empty();
                     --required)
                {
                    for (std::size_t at = 0; at < sprites.size();
                         ++at)
                    {
                        if (needShape
                            && !shapeFits(sprites[at], out))
                        {
                            continue;
                        }

                        if (matchCount(sprites[at], wants)
                            < required)
                        {
                            continue;
                        }

                        candidates.push_back(at);
                    }
                }
            }

            return weightedPick(
                sprites, candidates, nextRandom(rng));
        }

        void fillBaseCell(
            const std::vector<Sprite> &base,
            const std::array<bool, 4> &edges,
            const std::int32_t column,
            const std::int32_t row,
            std::uint32_t &rng,
            TilesetPreview &preview)
        {
            std::array<bool, 4> out{};
            NeighborWants wants{};

            for (std::size_t side = 0; side < 4; ++side)
            {
                const auto nearColumn =
                    column + kDeltaColumn[side];
                const auto nearRow = row + kDeltaRow[side];

                out[side] =
                    regionOutside(nearColumn, nearRow, edges);

                if (!inLattice(nearColumn, nearRow))
                {
                    continue;
                }

                const auto held =
                    preview.base[cellIndex(nearColumn, nearRow)];

                if (held >= 0)
                {
                    wants.socket[side] =
                        base[static_cast<std::size_t>(held)]
                            .sockets[kOpposite[side]];
                }
            }

            preview.base[cellIndex(column, row)] =
                static_cast<std::int32_t>(
                    chooseBase(base, out, wants, rng));
        }

        void fillBase(
            const std::vector<Sprite> &base,
            const std::array<bool, 4> &edges,
            std::uint32_t &rng,
            TilesetPreview &preview)
        {
            for (std::int32_t ring = 0; ring <= kPreviewColumns;
                 ++ring)
            {
                for (std::int32_t row = 0; row < kPreviewRows;
                     ++row)
                {
                    for (std::int32_t column = 0;
                         column < kPreviewColumns;
                         ++column)
                    {
                        const auto spread = std::max(
                            std::abs(
                                column - kPreviewCenterColumn),
                            std::abs(row - kPreviewCenterRow));
                        const auto at = cellIndex(column, row);

                        if (spread != ring || preview.outside[at]
                            || preview.base[at] >= 0)
                        {
                            continue;
                        }

                        fillBaseCell(
                            base, edges, column, row, rng, preview);
                    }
                }
            }
        }

        [[nodiscard]] bool decorFits(
            const std::vector<Sprite> &sprites,
            const std::size_t candidate,
            const tileset::SpriteId baseId,
            const std::int32_t west,
            const std::int32_t north,
            const bool eastHasBase,
            const bool southHasBase)
        {
            const auto &sprite = sprites[candidate];

            if (std::ranges::find(sprite.on, baseId)
                == sprite.on.end())
            {
                return false;
            }

            const auto wanted =
                west >= 0
                    ? sprites[static_cast<std::size_t>(west)]
                          .sockets[kEast]
                    : kOpenSocket;

            if (sprite.sockets[kWest] != wanted)
            {
                return false;
            }

            const auto above =
                north >= 0
                    ? sprites[static_cast<std::size_t>(north)]
                          .sockets[kSouth]
                    : kOpenSocket;

            if (sprite.sockets[kNorth] != above)
            {
                return false;
            }

            if (!eastHasBase
                && sprite.sockets[kEast] != kOpenSocket)
            {
                return false;
            }

            return southHasBase
                   || sprite.sockets[kSouth] == kOpenSocket;
        }

        void placeDecorCell(
            const tileset::Tileset &data,
            const tileset::Layer &layer,
            const std::size_t layerAt,
            const std::int32_t column,
            const std::int32_t row,
            const std::uint32_t seed,
            const std::array<bool, 4> &edges,
            const TilesetPreview &preview,
            std::array<std::int32_t, kPreviewCells> &decor)
        {
            const auto at = cellIndex(column, row);
            const auto hash = std::rotl(
                cellHash(column, row, seed),
                static_cast<int>((layerAt * 13) % 64));
            const auto west = column > 0 ? decor[at - 1] : -1;
            const auto north =
                row > 0
                    ? decor
                          [at
                           - static_cast<std::size_t>(
                               kPreviewColumns)]
                    : -1;
            const auto forced =
                (west >= 0
                 && layer.sprites[static_cast<std::size_t>(west)]
                            .sockets[kEast]
                        != kOpenSocket)
                || (north >= 0
                    && layer
                               .sprites[static_cast<std::size_t>(
                                   north)]
                               .sockets[kSouth]
                           != kOpenSocket);

            if (!forced && (hash >> 32) % 256 >= layer.density)
            {
                return;
            }

            const auto baseId =
                data.layers[0]
                    .sprites[static_cast<std::size_t>(
                        preview.base[at])]
                    .id;
            const bool eastHasBase =
                !regionOutside(column + 1, row, edges);
            const bool southHasBase =
                !regionOutside(column, row + 1, edges);

            std::vector<std::size_t> candidates{};

            for (std::size_t sprite = 0;
                 sprite < layer.sprites.size();
                 ++sprite)
            {
                if (decorFits(
                        layer.sprites,
                        sprite,
                        baseId,
                        west,
                        north,
                        eastHasBase,
                        southHasBase))
                {
                    candidates.push_back(sprite);
                }
            }

            if (candidates.empty())
            {
                return;
            }

            decor[at] = static_cast<std::int32_t>(
                weightedPick(layer.sprites, candidates, hash));
        }

        void scatterDecor(
            const tileset::Tileset &data,
            const std::size_t layerAt,
            const std::optional<std::size_t> pinned,
            const std::uint32_t seed,
            const std::array<bool, 4> &edges,
            TilesetPreview &preview)
        {
            const auto &layer = data.layers[layerAt];
            auto &decor = preview.decor[layerAt - 1];

            if (layer.sprites.empty())
            {
                return;
            }

            const auto center = cellIndex(
                kPreviewCenterColumn, kPreviewCenterRow);

            if (pinned.has_value())
            {
                decor[center] =
                    static_cast<std::int32_t>(*pinned);
            }

            for (std::int32_t row = 0; row < kPreviewRows; ++row)
            {
                for (std::int32_t column = 0;
                     column < kPreviewColumns;
                     ++column)
                {
                    const auto at = cellIndex(column, row);

                    if (preview.base[at] < 0 || decor[at] >= 0)
                    {
                        continue;
                    }

                    placeDecorCell(
                        data,
                        layer,
                        layerAt,
                        column,
                        row,
                        seed,
                        edges,
                        preview,
                        decor);
                }
            }
        }
    }

    TilesetPreview buildTilesetPreview(
        const tileset::Tileset &data,
        const std::size_t layer,
        const std::size_t sprite,
        const std::uint32_t seed)
    {
        TilesetPreview preview{};

        preview.base.fill(-1);

        for (std::size_t at = 1; at < data.layers.size(); ++at)
        {
            preview.decor.emplace_back().fill(-1);
        }

        if (layer >= data.layers.size()
            || sprite >= data.layers[layer].sprites.size()
            || data.layers[0].sprites.empty())
        {
            return preview;
        }

        const auto &selected = data.layers[layer].sprites[sprite];
        std::array<bool, 4> edges{};

        for (std::size_t side = 0; side < 4; ++side)
        {
            edges[side] = selected.sockets[side] == kEdgeSocket;
        }

        for (std::int32_t row = 0; row < kPreviewRows; ++row)
        {
            for (std::int32_t column = 0;
                 column < kPreviewColumns;
                 ++column)
            {
                preview.outside[cellIndex(column, row)] =
                    regionOutside(column, row, edges);
            }
        }

        auto rng = seed ^ 0x9E3779B9U;

        if (rng == 0)
        {
            rng = 1;
        }

        const auto center =
            cellIndex(kPreviewCenterColumn, kPreviewCenterRow);
        const auto &base = data.layers[0].sprites;

        if (layer == 0)
        {
            preview.base[center] =
                static_cast<std::int32_t>(sprite);
        }
        else
        {
            std::vector<std::size_t> allowed{};

            for (std::size_t at = 0; at < base.size(); ++at)
            {
                if (std::ranges::find(selected.on, base[at].id)
                    != selected.on.end())
                {
                    allowed.push_back(at);
                }
            }

            if (allowed.empty())
            {
                preview.centerBaseMissing = true;
            }
            else
            {
                preview.base[center] = static_cast<std::int32_t>(
                    weightedPick(base, allowed, nextRandom(rng)));
            }
        }

        fillBase(base, edges, rng, preview);

        for (std::size_t decorLayer = 1;
             decorLayer < data.layers.size();
             ++decorLayer)
        {
            const auto pinned =
                decorLayer == layer && !preview.centerBaseMissing
                    ? std::optional<std::size_t>{sprite}
                    : std::optional<std::size_t>{};

            scatterDecor(
                data, decorLayer, pinned, seed, edges, preview);
        }

        return preview;
    } // GCOVR_EXCL_LINE

}
