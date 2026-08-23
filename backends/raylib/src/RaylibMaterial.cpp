#include "RaylibMaterial.hpp"

namespace antwika::gfx::raylib
{

    namespace
    {
        constexpr int kSurfaceMapSlot = MATERIAL_MAP_METALNESS;

        constexpr int kShadowMapSlot = MATERIAL_MAP_NORMAL;

        constexpr int kLampShadowsSlot = MATERIAL_MAP_ROUGHNESS;

        constexpr int kSightMapSlot = MATERIAL_MAP_OCCLUSION;
    }

    RaylibMaterial::RaylibMaterial()
        : material(LoadMaterialDefault()),
          defaultShader(material.shader),
          defaultTexture(material.maps[MATERIAL_MAP_DIFFUSE].texture),
          defaultSurfaceMap(
              material.maps[kSurfaceMapSlot].texture),
          plainSurfaceMap(createNeutralSurfaceMap()),
          defaultShadowMap(material.maps[kShadowMapSlot].texture),
          defaultLampShadows(
              material.maps[kLampShadowsSlot].texture),
          defaultSightMap(material.maps[kSightMapSlot].texture),
          openShadows(createUnoccludedShadowMap())
    {
    }

    RaylibMaterial::~RaylibMaterial()
    {
        restoreDefaults();

        UnloadTexture(plainSurfaceMap);
        UnloadTexture(openShadows);
        UnloadMaterial(material);
    }

    ::Texture2D RaylibMaterial::createNeutralSurfaceMap()
    {
        const ::Color plain{.r = 0, .g = 0, .b = 0, .a = 255};
        ::Image image = GenImageColor(1, 1, plain);
        const auto texture = LoadTextureFromImage(image);

        UnloadImage(image);

        return texture;
    }

    void RaylibMaterial::setTint(Color tintColor) noexcept
    {
        material.maps[MATERIAL_MAP_DIFFUSE].color = ::Color{
            .r = tintColor.red,
            .g = tintColor.green,
            .b = tintColor.blue,
            .a = tintColor.alpha};
    }

    void RaylibMaterial::setTexture(const ::Texture2D *texture) noexcept
    {
        material.maps[MATERIAL_MAP_DIFFUSE].texture =
            texture == nullptr ? defaultTexture : *texture;
    }

    void RaylibMaterial::setSurfaceMap(
        const ::Texture2D *texture) noexcept
    {
        material.maps[kSurfaceMapSlot].texture =
            texture == nullptr ? plainSurfaceMap : *texture;
    }

    ::Texture2D RaylibMaterial::createUnoccludedShadowMap()
    {
        const ::Color open{.r = 255, .g = 255, .b = 255, .a = 255};
        ::Image image = GenImageColor(1, 1, open);
        const auto texture = LoadTextureFromImage(image);

        UnloadImage(image);

        return texture;
    }

    void RaylibMaterial::setShadowMap(
        const ::Texture2D *texture) noexcept
    {
        material.maps[kShadowMapSlot].texture =
            texture == nullptr ? openShadows : *texture;
    }

    void RaylibMaterial::setLampShadows(
        const ::Texture2D *texture) noexcept
    {
        material.maps[kLampShadowsSlot].texture =
            texture == nullptr ? openShadows : *texture;
    }

    void RaylibMaterial::setShader(const ::Shader *shader) noexcept
    {
        material.shader = shader == nullptr ? defaultShader : *shader;
    }

    void RaylibMaterial::restoreDefaults() noexcept
    {
        setTexture(nullptr);
        setShader(nullptr);

        material.maps[kSurfaceMapSlot].texture = defaultSurfaceMap;
        material.maps[kShadowMapSlot].texture = defaultShadowMap;
        material.maps[kLampShadowsSlot].texture =
            defaultLampShadows;
        material.maps[kSightMapSlot].texture = defaultSightMap;
    }

    const ::Material &RaylibMaterial::getRawHandle() const noexcept
    {
        return material;
    }

}
