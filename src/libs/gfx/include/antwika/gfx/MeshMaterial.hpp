#pragma once

#include "antwika/gfx/BlendMode.hpp"
#include "antwika/gfx/Color.hpp"
#include "antwika/gfx/IShader.hpp"
#include "antwika/gfx/ITexture.hpp"

namespace antwika::gfx
{

    struct MeshMaterial final
    {
        const ITexture *texture = nullptr;

        const ITexture *materialMapTexture = nullptr;

        const ITexture *shadowMapTexture = nullptr;

        const ITexture *pointLightShadowAtlasTexture = nullptr;


        const IShader *shader = nullptr;

        Color tintColor{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        BlendMode blend = BlendMode::Opaque;

        [[nodiscard]] bool operator==(
            const MeshMaterial &other) const = default;
    };

}
