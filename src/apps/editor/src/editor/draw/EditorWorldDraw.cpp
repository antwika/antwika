#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/text/TextLayout.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::drawWorldView(
        const ui::Frame &frame,
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        const auto worldFrom = clockSource.currentTime();
        const auto modelMatrix = worldRotation();
        const auto camera = worldCamera();

        if (decor::hasAnimatedDecor(document.map.decor)
            && tick % decor::kDecorPaceTick == 0)
        {
            rebuildDecorMesh();
        }

        const auto pile = [&]
        {
            for (const auto &piece : worldMeshes.solid())
            {
                viewportRenderer.drawMesh(
                    *piece,
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.texture(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.texture(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.hiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.lampShadows(),
                        .shader = &worldShader.program()});
            }

            if (worldMeshes.decor() != nullptr)
            {
                viewportRenderer.drawMesh(
                    *worldMeshes.decor(),
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.keyed(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.keyed(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.hiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.lampShadows(),
                        .shader = &worldShader.program()});
            }

            for (const auto &piece : worldMeshes.water())
            {
                viewportRenderer.drawMesh(
                    *piece,
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.texture(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.texture(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.hiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.lampShadows(),
                        .shader = &worldShader.program()});
            }
        };

        scenePass.draw(
            viewportRenderer,
            worldShader.program(),
            play.playing ? kPlayBackgroundColor : kEditorBackgroundColor,
            pile,
            [&]
            {
                if (play.playing)
                {
                    const auto stoodPosition =
                        play.game->world().get<component::Position>(
                            play.game->player());

                    const auto ground = collision::groundHeightUnderFootprint(
                        worldMeshes.cells(), stoodPosition.x, stoodPosition.z,
                        stoodPosition.y);

                    if (ground.has_value())
                    {
                        sprites.drawShadow(
                            viewportRenderer,
                            camera,
                            modelMatrix,
                            antwika::gfx::Vec3{
                                stoodPosition.x, *ground + 0.02F,
                                stoodPosition.z});
                    }

                    for (const auto entity :
                         play.game->world()
                             .view<
                                 component::Position,
                                 component::AnimationState,
                                 component::RosterIndex>())
                    {
                        const auto index =
                            play.game->world()
                                .get<component::RosterIndex>(entity)
                                .index;

                        if (index >= characterView.skins().size())
                        {
                            continue;
                        }

                        drawSprite(
                            camera,
                            modelMatrix,
                            characterView.skinTexture(index),
                            play.game->world().get<component::Position>(
                                entity),
                            play.game->world()
                                .get<component::AnimationState>(
                                    entity));
                    }
                }
            });

        drawWorldOverlays(frame, camera, modelMatrix);

        if (play.playing && tick < caption.untilTick)
        {
            viewportRenderer.drawRect(
                antwika::gfx::RectF(
                    {8.0F,
                     static_cast<float>(camera::kCanvasSize.height) - 30.0F},
                    {static_cast<float>(camera::kCanvasSize.width) - 16.0F,
                     24.0F}),
                gfx::Color{.red = 14, .green = 14, .blue = 20});
            viewportRenderer.drawText(
                {16.0F,
                 static_cast<float>(camera::kCanvasSize.height) - 22.0F},
                caption.name + " - "
                    + caption.line.substr(
                        0,
                        std::min<std::size_t>(
                            caption.line.size(),
                            (tick - caption.start)
                                / antwika::editor::
                                    kCaptionCharTicks)),
                1.0F,
                kTextColor);
        }

        if (play.playing && play.titleScreenUp)
        {
            const auto slash = document.path().find_last_of("/\\");
            const auto fileName =
                slash == std::string::npos
                       ? document.path()
                       : document.path().substr(slash + 1);

            viewportRenderer.drawRect(
                antwika::gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(camera::kCanvasSize.width),
                     static_cast<float>(camera::kCanvasSize.height)}),
                kPlayBackgroundColor);
            viewportRenderer.drawText(
                {camera::kCanvasSize.width / 2.0F - 60.0F,
                 camera::kCanvasSize.height / 2.0F - 12.0F},
                fileName,
                1.0F,
                kTextColor);
            viewportRenderer.drawText(
                {camera::kCanvasSize.width / 2.0F - 60.0F,
                 camera::kCanvasSize.height / 2.0F + 4.0F},
                "press any key",
                1.0F,
                kGridLineColor);
        }

        if (play.playing)
        {
            const auto rateText =
                time::formatFrameRate(meters.frameRate.perSecond());
            const auto workText =
                time::formatFrameTime(meters.workRate.averageFrameTime())
                + " l "
                + time::formatFrameTime(meters.lampRate.averageFrameTime())
                + " c "
                + time::formatFrameTime(meters.sightRate.averageFrameTime())
                + " h "
                + time::formatFrameTime(meters.hideRate.averageFrameTime());

            viewportRenderer.drawText(
                {static_cast<float>(camera::kCanvasSize.width)
                     - static_cast<float>(
                         antwika::text::textSize(rateText, 1)
                             .width)
                     - 4.0F,
                 4.0F},
                rateText,
                1,
                kTextColor);
            viewportRenderer.drawText(
                {static_cast<float>(camera::kCanvasSize.width)
                     - static_cast<float>(
                         antwika::text::textSize(workText, 1)
                             .width)
                     - 4.0F,
                 16.0F},
                workText,
                1,
                kGridLineColor);
        }

        drawColorPicker();

        viewportRenderer.fillLetterbox(gfx::Color{});

        if (!play.playing)
        {
            antwika::ui::paint(
                viewportRenderer.nativeRenderer(), frame.drawList);
            drawToolHint(frame);
            drawCanvasHint();
        }

        meters.worldRate.record(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                clockSource.currentTime() - worldFrom));

        viewportRenderer.present();

        recordFrameWork(startedAt);
    }

}
