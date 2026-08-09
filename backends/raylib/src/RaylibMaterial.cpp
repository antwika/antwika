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

}
