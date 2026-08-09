#pragma once

#include <array>

#include <antwika/gfx/ITexture.hpp>

#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    using antwika::gfx::ITexture;

    struct AtlasTextures final
    {
        const ITexture &oneByOne;

        const ITexture &twoByTwo;

        const ITexture &threeByThree;

        const ITexture &walker;

        const AtlasSpecs &specs;

        [[nodiscard]] const ITexture &of(AtlasKind kind) const noexcept
        {
            const std::array<const ITexture *, kAtlasKindCount> sheets{
                &oneByOne, &twoByTwo, &threeByThree};

            return *sheets[atlasKindIndex(kind) % kAtlasKindCount];
        }
    };

}
