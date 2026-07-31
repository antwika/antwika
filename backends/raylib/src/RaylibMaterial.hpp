#pragma once

#include <raylib.h>

#include <antwika/gfx/Color.hpp>

namespace antwika::gfx::raylib
{

    /**
     * @brief The one material every mesh this backend draws is drawn
     * with.
     *
     * antwika::gfx has no material of its own, and deliberately so: a
     * mesh carries its own vertex colours and a draw carries a tint, and
     * that is the whole of what IRenderer3D lets a caller say about
     * appearance.
     * raylib's DrawMesh() nevertheless insists on being handed one, so
     * this wraps LoadMaterialDefault() -- raylib's own unlit shader over
     * a 1x1 white texture, which multiplies the tint into each vertex
     * colour and is therefore exactly the appearance IRenderer3D
     * describes.
     *
     * It is a class rather than a member of RaylibRenderer because a
     * material must be unloaded while the GL context still exists, which
     * is the same rule RaylibTexture and RaylibMesh live by, and because
     * keeping the raylib type out of RaylibRenderer.hpp keeps that
     * header free of raylib's global names.
     */
    class RaylibMaterial final
    {
    public:
        /**
         * @brief Load raylib's default material.
         *
         * Only valid while a window is open, since the shader and the
         * placeholder texture it names belong to the GL context.
         */
        RaylibMaterial();

        RaylibMaterial(const RaylibMaterial &) = delete;
        RaylibMaterial(RaylibMaterial &&) = delete;

        RaylibMaterial &operator=(const RaylibMaterial &) = delete;
        RaylibMaterial &operator=(RaylibMaterial &&) = delete;

        /**
         * @brief Unload the material.
         *
         * Frees only this object's own map array: raylib owns the
         * default shader and the default texture the maps name, and
         * UnloadMaterial() leaves both alone.
         */
        ~RaylibMaterial();

        /**
         * @brief Choose what the next draw multiplies vertex colours by.
         * @param tint The colour and alpha to modulate with.
         */
        void setTint(Color tint) noexcept;

        /**
         * @brief Get the material to draw with.
         * @return The material, valid for as long as this object is.
         */
        [[nodiscard]] const ::Material &raw() const noexcept;

    private:
        ::Material material;
    };

} // namespace antwika::gfx::raylib
