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
        const auto worldFrom = clockSource.getCurrentTime();
        const auto modelMatrix = worldRotation();
        const auto camera = worldCamera();

        if (decor::hasAnimatedDecor(document.map.decor)
            && tick % decor::kDecorPaceTick == 0)
        {
            rebuildDecorMesh();
        }

        const auto pile = [&]
        {
            for (const auto &piece : worldMeshes.getSolid())
            {
                viewportRenderer.drawMesh(
                    *piece,
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.getTexture(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.getTexture(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.getHiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.getLampShadows(),
                        .shader = &worldShader.getProgram()});
            }

            if (worldMeshes.getDecor() != nullptr)
            {
                viewportRenderer.drawMesh(
                    *worldMeshes.getDecor(),
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.getKeyed(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.getKeyed(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.getHiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.getLampShadows(),
                        .shader = &worldShader.getProgram()});
            }

            for (const auto &piece : worldMeshes.getWater())
            {
                viewportRenderer.drawMesh(
                    *piece,
                    modelMatrix,
                    camera,
                    antwika::gfx::MeshMaterial{
                        .texture = atlasSheets.getTexture(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.getTexture(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.getHiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.getLampShadows(),
                        .shader = &worldShader.getProgram()});
            }
        };

        scenePass.draw(
            viewportRenderer,
            worldShader.getProgram(),
            play.playing ? kPlayBackgroundColor : kEditorBackgroundColor,
            pile,
            [&]
            {
                if (play.playing)
                {
                    const auto stoodPosition =
                        play.game->getWorld().get<component::Position>(
                            play.game->getPlayer());

                    const auto ground = collision::getGroundHeightUnderFootprint(
                        worldMeshes.getCells(), stoodPosition.x, stoodPosition.z,
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
                         play.game->getWorld()
                             .view<
                                 component::Position,
                                 component::AnimationState,
                                 component::RosterIndex>())
                    {
                        const auto index =
                            play.game->getWorld()
                                .get<component::RosterIndex>(entity)
                                .index;

                        if (index >= characterView.getSkins().size())
                        {
                            continue;
                        }

                        drawSprite(
                            camera,
                            modelMatrix,
                            characterView.getSkinTexture(index),
                            play.game->getWorld().get<component::Position>(
                                entity),
                            play.game->getWorld()
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
            const auto slash = document.getPath().find_last_of("/\\");
            const auto fileName =
                slash == std::string::npos
                       ? document.getPath()
                       : document.getPath().substr(slash + 1);

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
                time::getFormatFrameRate(meters.frameRate.getPerSecond());
            const auto workText =
                time::getFormatFrameTime(meters.workRate.getAverageFrameTime())
                + " l "
                + time::getFormatFrameTime(meters.lampRate.getAverageFrameTime())
                + " c "
                + time::getFormatFrameTime(meters.sightRate.getAverageFrameTime())
                + " h "
                + time::getFormatFrameTime(meters.hideRate.getAverageFrameTime());

            viewportRenderer.drawText(
                {static_cast<float>(camera::kCanvasSize.width)
                     - static_cast<float>(
                         antwika::text::getTextSize(rateText, 1)
                             .width)
                     - 4.0F,
                 4.0F},
                rateText,
                1,
                kTextColor);
            viewportRenderer.drawText(
                {static_cast<float>(camera::kCanvasSize.width)
                     - static_cast<float>(
                         antwika::text::getTextSize(workText, 1)
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
                clockSource.getCurrentTime() - worldFrom));

        viewportRenderer.present();

        recordFrameWork(startedAt);
    }

}
