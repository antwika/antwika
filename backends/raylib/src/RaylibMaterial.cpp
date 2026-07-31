#include "RaylibMaterial.hpp"

namespace antwika::gfx::raylib
{

    RaylibMaterial::RaylibMaterial()
        : material(LoadMaterialDefault())
    {
    }

    RaylibMaterial::~RaylibMaterial()
    {
        UnloadMaterial(material);
    }

    void RaylibMaterial::setTint(Color tint) noexcept
    {
        // DrawMesh uploads this as the shader's colDiffuse.
        // The default shader multiplies it into the vertex colour.
        // It multiplies the texture in as well.
        // That texture is 1x1 white, so it changes nothing.
        // What reaches a pixel is the vertex colour times this tint.
        // Which is exactly what IRenderer3D::drawMesh promises.
        material.maps[MATERIAL_MAP_DIFFUSE].color = ::Color{
            .r = tint.red,
            .g = tint.green,
            .b = tint.blue,
            .a = tint.alpha};
    }

    const ::Material &RaylibMaterial::raw() const noexcept
    {
        return material;
    }

} // namespace antwika::gfx::raylib
