#include "antwika/autotile/MissingArt.hpp"

#include <cstdint>

#include <antwika/tileset/Sprite.hpp>

namespace antwika::autotile
{

    namespace
    {
        [[nodiscard]] const tileset::Sprite *spriteAtRow(
            const tileset::Tileset &set, const std::uint32_t atlasRow)
        {
            auto row = atlasRow;

            for (const auto &layer : set.layers)
            {
                const auto size =
                    static_cast<std::uint32_t>(layer.sprites.size());

                if (row < size)
                {
                    return &layer.sprites[row];
                }

                row -= size;
            }

            return nullptr;
        }
    }

    bool artMissing(
        const TileDraw &draw,
        const tileset::Tileset &set,
        const tileset::AtlasIndex &index)
    {
        if (draw.kind != DrawKind::Sprite)
        {
            return draw.kind != DrawKind::Shade;
        }

        if (draw.atlasRow >= index.rows)
        {
            return true;
        }

        const auto *sprite = spriteAtRow(set, draw.atlasRow);

        return sprite == nullptr
               || tileset::isBlank(sprite->frames[draw.frame]);
    }

}
