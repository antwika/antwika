#include "antwika/atlas_editor/AtlasMeta.hpp"

#include <cstdint>
#include <optional>

#include "antwika/atlas_editor/SpriteGuides.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"

namespace antwika::atlas_editor
{

    AtlasMeta metaFor(const TileGrid tile, const Size sheet) noexcept
    {
        AtlasMeta meta;
        meta.sprite = Size{.width = tile.width, .height = tile.height};

        const auto outlines = guidesForTile(tile);

        if (outlines.has_value())
        {
            meta.pivot = outlines->pivot;
            meta.isometric = outlines->footprint;
        }
        else
        {
            meta.kind = AtlasKind::Flat;
            meta.pivot = Point{
                .x = static_cast<std::int32_t>(tile.width / 2),
                .y = static_cast<std::int32_t>(tile.height)};
        }

        return counted(meta, sheet);
    }

    TileGrid tilesOf(const AtlasMeta &meta) noexcept
    {
        return TileGrid{
            .width = meta.sprite.width, .height = meta.sprite.height};
    }

    std::optional<SpriteGuides> guidesOf(const AtlasMeta &meta) noexcept
    {
        if (meta.kind != AtlasKind::Isometric
            || meta.isometric.width == 0 || meta.isometric.height == 0)
        {
            return std::nullopt;
        }

        return SpriteGuides{
            .pivot = meta.pivot, .footprint = meta.isometric};
    }

}
