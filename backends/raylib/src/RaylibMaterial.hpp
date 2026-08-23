#pragma once

#include <raylib.h>

#include <antwika/gfx/Color.hpp>

namespace antwika::gfx::raylib
{

    class RaylibMaterial final
    {
    public:
        RaylibMaterial();

        RaylibMaterial(const RaylibMaterial &) = delete;
        RaylibMaterial(RaylibMaterial &&) = delete;

        RaylibMaterial &operator=(const RaylibMaterial &) = delete;
        RaylibMaterial &operator=(RaylibMaterial &&) = delete;

        ~RaylibMaterial();

        void setTint(Color tintColor) noexcept;

        void setTexture(const ::Texture2D *texture) noexcept;

        void setSurfaceMap(const ::Texture2D *texture) noexcept;

        void setShadowMap(const ::Texture2D *texture) noexcept;

        void setLampShadows(const ::Texture2D *texture) noexcept;

        void setShader(const ::Shader *shader) noexcept;

        void restoreDefaults() noexcept;

        [[nodiscard]] const ::Material &getRawHandle() const noexcept;

    private:
        [[nodiscard]] static ::Texture2D createNeutralSurfaceMap();

        [[nodiscard]] static ::Texture2D createUnoccludedShadowMap();

        ::Material material;
        ::Shader defaultShader;
        ::Texture2D defaultTexture;
        ::Texture2D defaultSurfaceMap;
        ::Texture2D plainSurfaceMap;
        ::Texture2D defaultShadowMap;
        ::Texture2D defaultLampShadows;
        ::Texture2D defaultSightMap;
        ::Texture2D openShadows;
    };

}
