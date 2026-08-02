#pragma once

#include <array>

#include <antwika/gfx/ITexture.hpp>

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    using antwika::gfx::ITexture;

    /**
     * @brief The three uploaded sheets, one per AtlasKind.
     *
     * A struct of borrowed references rather than an array, for the
     * reason RenderSetup is: three same-typed references in a row are
     * distinguishable only by where they sit, and a swapped pair would
     * draw every farm from the storehouse's sheet.
     *
     * Every member must have come from the one renderer that draws
     * them, and must outlive whatever this is handed to.
     */
    struct AtlasTextures
    {
        /** @brief Ground, roads, walkers and 1x1 buildings. */
        const ITexture &oneByOne;

        /** @brief The 2x2 buildings. */
        const ITexture &twoByTwo;

        /** @brief The 3x3 buildings. */
        const ITexture &threeByThree;

        /**
         * @brief Get the sheet an AtlasKind names.
         *
         * The one crossing between the address map's names and the
         * uploaded pictures, so a sprite and the sheet it is blitted
         * from cannot be chosen by two different decisions.
         *
         * @param kind The sheet to fetch.
         * @return The texture it was uploaded to.
         */
        [[nodiscard]] const ITexture &of(AtlasKind kind) const noexcept
        {
            const std::array<const ITexture *, kAtlasKindCount> sheets{
                &oneByOne, &twoByTwo, &threeByThree};

            return *sheets[atlasKindIndex(kind) % kAtlasKindCount];
        }
    };

} // namespace antwika::game
