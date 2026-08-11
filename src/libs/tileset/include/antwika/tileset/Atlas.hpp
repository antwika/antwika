#pragma once

#include <cstdint>
#include <vector>

#include <antwika/geometry/Rect.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>

#include "antwika/tileset/Sprite.hpp"
#include "antwika/tileset/Tileset.hpp"

namespace antwika::tileset
{

    inline constexpr std::int32_t kAtlasWidth =
        kMaxFrames * kSpriteSide;

    struct AtlasIndex final
    {
        std::vector<std::uint32_t> layerRowOffsets{};
        std::uint32_t rows = 0;

        [[nodiscard]] bool operator==(
            const AtlasIndex &other) const = default;
    };

    /**
     * @brief Maps each layer to its first row in the baked atlas.
     *
     * @param set The tileset the atlas is baked from.
     * @return One row offset per layer beside the total row count.
     *
     * Ensures: layers stack in order with layer 0 first, one row per
     *          sprite.
     */
    [[nodiscard]] AtlasIndex atlasIndexOf(const Tileset &set);

    /**
     * @brief The pixel rectangle one frame occupies in the atlas.
     *
     * @param atlasRow The sprite's row in the atlas.
     * @param frame The frame slot, 0 to kMaxFrames - 1.
     * @return The kSpriteSide square at that row and frame.
     */
    [[nodiscard]] geometry::Rect atlasSource(
        std::uint32_t atlasRow, std::uint8_t frame) noexcept;

    /**
     * @brief Bakes every sprite into one tinted bitmap.
     *
     * @param set The tileset to bake.
     * @param ink The color ink pixels take.
     * @param paper The color paper pixels take.
     * @return A kAtlasWidth wide bitmap, one sprite row per sprite.
     *
     * Ensures: ink and paper pixels come out opaque in their colors,
     *          and blank pixels and frame slots past a sprite's
     *          frameCount stay fully transparent.
     */
    [[nodiscard]] gfx::Bitmap bakeAtlas(
        const Tileset &set, gfx::Color ink, gfx::Color paper);

}
