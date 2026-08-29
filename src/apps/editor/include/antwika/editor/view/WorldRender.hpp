#pragma once

#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/render/AtlasSheets.hpp>
#include <antwika/render/CharacterSkins.hpp>
#include <antwika/render/LightPasses.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/Sprites.hpp>
#include <antwika/render/WorldMeshes.hpp>
#include <antwika/render/WorldShader.hpp>

namespace antwika::editor
{

    struct WorldRender final
    {
        gfx::ViewportRenderer &viewportRenderer;

        render::AtlasSheets &atlasSheets;

        render::WorldMeshes &worldMeshes;

        render::WorldShader &worldShader;

        render::Sprites &sprites;

        render::ScenePass &scenePass;

        render::LightPasses &lightPasses;

        render::CharacterSkins &characterSkins;
    };

}
