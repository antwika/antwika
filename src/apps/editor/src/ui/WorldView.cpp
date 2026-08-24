#include "antwika/editor/ui/WorldView.hpp"

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/RosterIndex.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/render/HealthBars.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/text/TextLayout.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/WorldCamera.hpp"
#include "antwika/editor/tools/ShapedCubes.hpp"
#include "antwika/editor/view/WorldSprites.hpp"

namespace
{

    constexpr antwika::gfx::Color kSightPointColor{
        .red = 255, .green = 64, .blue = 64, .alpha = 255};

    constexpr float kSightPointSide = 3.0F;

    constexpr antwika::gfx::Color kOriginPointColor{
        .red = 255, .green = 255, .blue = 255, .alpha = 255};

}

namespace antwika::editor
{

    bool WorldView::claims(
        const map::View shownView, const bool playing) const noexcept
    {
        return playing || shownView == map::View::World;
    }

    bool WorldView::offersPaint(const map::Paint) const noexcept
    {
        return false;
    }

    std::string WorldView::getStatusText(
        const ViewContext &viewContext) const
    {
        return std::string(
                   viewContext.workbench.preferences.tool == map::Tool::Brush
                         ? "1 world - lmb adds - rmb takes - "
                         "f5 plays"
                   : viewContext.workbench.preferences.tool == map::Tool::Lamp
                       ? "1 world - lmb sets a lamp of the "
                         "ink chosen - rmb takes"
                   : viewContext.workbench.preferences.tool == map::Tool::Start
                       ? "1 world - lmb sets the start cube "
                         "- rmb takes it"
                   : viewContext.workbench.preferences.tool == map::Tool::Exit
                       ? "1 world - lmb sets the exit cube "
                         "- rmb takes it"
                   : viewContext.workbench.preferences.tool == map::Tool::Stamp
                       ? "1 world - drag copies cubes - lmb "
                         "sets them down - rmb drops them"
                   : viewContext.workbench.preferences.tool == map::Tool::Figure
                       ? "1 world - lmb stands the chosen "
                         "figure here, again adds a stop - "
                         "rmb takes it away"
                   : viewContext.workbench.preferences.tool == map::Tool::PressurePlate
                       ? "1 world - lmb sets the plate, then "
                         "picks the cubes it sways - rmb "
                         "takes it"
                   : viewContext.workbench.preferences.tool == map::Tool::Food
                       ? "1 world - lmb lays food to pick up "
                         "- rmb takes it"
                   : viewContext.workbench.preferences.tool == map::Tool::Water
                       ? "1 world - lmb lays water to pick up "
                         "- rmb takes it"
                   : viewContext.workbench.preferences.tool == map::Tool::Eraser
                       ? "1 world - lmb clears cubes - drag "
                         "sweeps them away"
                       : "1 world - lmb picks a tile - rmb "
                         "takes")
               + " - wheel zooms - g - c "
               + std::string(
                   worldEdit.cornerJoining
                           == solver::CornerSeams::Included
                            ? "on"
                            : "off")
               + " - level " + std::to_string(worldEdit.editLevel);
    }


    void WorldView::draw(
        const ViewContext &viewContext, const ui::Frame &frame)
    {
        auto &document = viewContext.document;
        auto &play = viewContext.play;
        auto &cameraRig = viewContext.cameraRig;
        const auto &caption = viewContext.caption;
        auto &worldMeshes = viewContext.render.worldMeshes;
        auto &viewportRenderer = viewContext.render.viewportRenderer;
        auto &atlasSheets = viewContext.render.atlasSheets;
        auto &lightPasses = viewContext.render.lightPasses;
        auto &worldShader = viewContext.render.worldShader;
        auto &scenePass = viewContext.render.scenePass;
        auto &sprites = viewContext.render.sprites;
        auto &rosterSkins = viewContext.render.rosterSkins;
        auto &clockSource = viewContext.clockSource;
        auto &meters = viewContext.meters;
        const auto tick = viewContext.tick;

        const auto worldFrom = clockSource.getCurrentTime();
        const auto modelMatrix = getWorldRotation(play);
        const auto camera = getWorldCamera(play, cameraRig);

        if (decor::hasAnimatedDecor(document.map.decor)
            && tick % decor::kDecorPaceTick == 0)
        {
            worldMeshes.rebuildDecor(viewportRenderer, document.map, tick);
        }

        const auto clipMatrix = camera.getViewProjection() * modelMatrix;

        const auto pile = [&]
        {
            for (const auto &piece : worldMeshes.getSolid())
            {
                if (antwika::gfx::isBoxOutside(piece.box, clipMatrix))
                {
                    continue;
                }

                viewportRenderer.drawMesh(
                    *piece.mesh,
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
                        .texture = atlasSheets.getKeyedTexture(tilemap::Atlas::Floor),
                        .materialMapTexture = atlasSheets.getKeyedTexture(
                            tilemap::Atlas::Wall),
                        .shadowMapTexture = lightPasses.getHiding(),
                        .pointLightShadowAtlasTexture =
                            lightPasses.getLampShadows(),
                        .shader = &worldShader.getProgram()});
            }

            for (const auto &piece : worldMeshes.getWater())
            {
                if (antwika::gfx::isBoxOutside(piece.box, clipMatrix))
                {
                    continue;
                }

                viewportRenderer.drawMesh(
                    *piece.mesh,
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

                        if (index >= rosterSkins.getSheets().size())
                        {
                            continue;
                        }

                        drawSprite(
                            viewContext.render,
                            tick,
                            camera,
                            modelMatrix,
                            rosterSkins.getPicture(index),
                            play.game->getWorld().get<component::Position>(
                                entity),
                            play.game->getWorld()
                                .get<component::AnimationState>(
                                    entity));
                    }
                }
            });

        drawWorldOverlays(viewContext, frame, camera, modelMatrix);

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

        meters.worldRate.record(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                clockSource.getCurrentTime() - worldFrom));
    }

    std::vector<voxel::VoxelPosition> WorldView::getStampGhost(
        const ViewContext &,
        const voxel::VoxelPosition position) const
    {
        std::vector<voxel::VoxelPosition> positions;

        if (!stamp.voxels.empty())
        {
            const auto corner = antwika::voxel::cubeCornerOf(position);

            for (const auto &[offset, material] : stamp.voxels)
            {
                positions.push_back(
                    voxel::VoxelPosition{
                        .x = corner.x + offset.x,
                        .y = offset.y,
                        .z = corner.z + offset.z});
            }

            return positions;
        }

        if (!stamp.fromPosition.has_value())
        {
            return positions;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stamp.fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(position);

        for (auto x = std::min(a.x, b.x);
             x <= std::max(a.x, b.x);
             x += voxel::kCubeSide)
        {
            for (auto z = std::min(a.z, b.z);
                 z <= std::max(a.z, b.z);
                 z += voxel::kCubeSide)
            {
                positions.push_back(
                    voxel::VoxelPosition{.x = x, .y = a.y, .z = z});
            }
        }

        return positions;
    } // GCOVR_EXCL_LINE

    bool WorldView::isUpperSightOn(
        const ViewContext &viewContext) const
    {
        auto &play = viewContext.play;
        auto &worldMeshes = viewContext.render.worldMeshes;

        if (!play.playing)
        {
            return true;
        }

        const auto stoodPosition =
            play.game->getWorld().get<component::Position>(play.game->getPlayer());

        return !antwika::voxel::isCubeAbove(
            worldMeshes.getCells(),
            antwika::gfx::Vec3{
                stoodPosition.x, stoodPosition.y, stoodPosition.z},
            light::kSightClearance);
    }

    void WorldView::drawWorldOverlays(
        const ViewContext &viewContext,
        const ui::Frame &frame,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix)
    {
        auto &document = viewContext.document;
        auto &play = viewContext.play;
        auto &cameraRig = viewContext.cameraRig;
        const auto &preferences = viewContext.workbench.preferences;
        const auto &pointer = viewContext.workbench.pointer;
        auto &worldMeshes = viewContext.render.worldMeshes;
        auto &viewportRenderer = viewContext.render.viewportRenderer;
        auto &lightPasses = viewContext.render.lightPasses;
        auto &clockSource = viewContext.clockSource;
        auto &meters = viewContext.meters;

        if (!play.playing && overlays.stale)
        {
            overlays.gridLines = voxelmap::getLevelGridLines(
                document.map.voxels, antwika::voxel::getCubeTop(worldEdit.editLevel));
            overlays.topLines = voxelmap::getBuildableTopOutlines(
                document.map.voxels, antwika::voxel::getCubeTop(worldEdit.editLevel));

            const auto satisfiedSeamSet = solver::getSatisfiedSeams(
                worldMeshes.getFaces(),
                worldMeshes.getSolvedTiles(),
                document.map.rules,
                worldEdit.cornerJoining);

            overlays.seamsAboveLevel = solver::getCrossLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(worldEdit.editLevel));
            overlays.seamsAtLevel = solver::getSameLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(worldEdit.editLevel));
            overlays.stale = false;
        }

        const auto clipMatrix = camera.getViewProjection() * modelMatrix;

        {
            const auto ruled = [&](const auto &spans,
                                   const gfx::Color ink)
            {
                for (const auto &span : spans)
                {
                    const auto fromPoint =
                        voxelmap::getProjectToScreen(
                            clipMatrix,
                            camera::kCanvasSize,
                            span.fromPosition);
                    const auto toPoint =
                        voxelmap::getProjectToScreen(
                            clipMatrix,
                            camera::kCanvasSize,
                            span.toPosition);

                    if (fromPoint.has_value() && toPoint.has_value())
                    {
                        viewportRenderer.drawLine(*fromPoint, *toPoint, ink);
                    }
                }
            };

            if (play.playing)
            {
                for (const auto keyCell : document.map.markers.positionsOf(map::Marker::Key))
                {
                    if (!play.game->getGates().collectedKeyPositions.contains(
                            antwika::voxel::cubeCornerOf(keyCell)))
                    {
                        ruled(voxelmap::getCubeWireframe(keyCell), kRuleLineColor);
                    }
                }

                for (const auto doorCell : document.map.markers.positionsOf(map::Marker::Door))
                {
                    ruled(
                        voxelmap::getCubeWireframe(doorCell),
                        kCornerSeamLineColor);
                }

                for (const auto entity :
                     play.game->getWorld().view<antwika::component::Item>())
                {
                    const auto item =
                        play.game->getWorld().get<antwika::component::Item>(
                            entity);

                    ruled(
                        voxelmap::getCubeWireframe(item.position),
                        static_cast<component::ItemKind>(item.kind)
                                == component::ItemKind::Food
                                 ? kFoodBarColor
                                 : kWaterBarColor);
                }
            }

            for (const auto troubleCell : grow.troublePositions)
            {
                ruled(voxelmap::getCubeWireframe(troubleCell),
                kForbiddenMarkerColor);
            }

            if (play.playing && play.game->getPathGoal().has_value())
            {
                ruled(
                    voxelmap::getCubeWireframe(*play.game->getPathGoal()),
                    kPlacementPreviewColor);
            }

            if (!play.playing)
            {
                if (!lightPasses.getHiddenVoxels().empty())
                {
                    ruled(
                        voxelmap::getOccluderFootprints(
                            lightPasses.getHiddenVoxels()),
                        kLevelGridLineColor);
                }

                if (!cameraRig.freeLook && preferences.grid)
                {
                    ruled(overlays.gridLines, kLevelGridLineColor);
                }

                if (!cameraRig.freeLook)
                {
                    ruled(overlays.topLines, kCursorColor);
                }

                const auto going = voxelmap::getCellUnder(
                    camera,
                    modelMatrix,
                    camera::kCanvasSize,
                    pointer.pointerOnCanvas,
                    antwika::voxel::getCubeTop(worldEdit.editLevel));

                const auto steering =
                    cameraRig.orbiting
                    || (cameraRig.panning
                        && cameraRig.panGripPosition.has_value())
                    || play.game->wasdKeys() != input::DirectionKeys{}
                    || play.game->arrowKeys() != input::DirectionKeys{}
                    || worldEdit.descendHeld
                    || worldEdit.ascendHeld;

                if (going.has_value() && !cameraRig.freeLook
                    && preferences.showPlacementGhost && !steering
                    && !frame.interactions.pointerOverUi)
                {
                    if (preferences.tool == map::Tool::Stamp
                        && (stamp.fromPosition.has_value()
                            || !stamp.voxels.empty()))
                    {
                        for (const auto cube :
                             getStampGhost(viewContext, *going))
                        {
                            ruled(
                                voxelmap::getCubeWireframe(cube),
                                    kPlacementPreviewColor);
                        }
                    }
                    else if (worldPaint.shapeFromPosition.has_value())
                    {
                        for (const auto cube : getShapedCubes(
                                 *worldPaint.shapeFromPosition,
                                 *going,
                                 preferences.paint))
                        {
                            ruled(
                                voxelmap::getCubeWireframe(cube),
                                    kPlacementPreviewColor);
                        }
                    }
                    else
                    {
                        ruled(voxelmap::getCubeWireframe(*going),
                            kPlacementPreviewColor);
                    }
                }

                for (const auto lamp : document.map.lamps)
                {
                    ruled(light::getLampGizmoSpans(lamp), lamp.tintColor);
                }

                if (document.map.spawnCubePosition.has_value())
                {
                    ruled(
                        voxelmap::getCubeWireframe(
                            *document.map.spawnCubePosition),
                        kCornerFilledMarkerColor);
                }

                if (document.map.exitCubePosition.has_value())
                {
                    ruled(voxelmap::getCubeWireframe(
                            *document.map.exitCubePosition),
                        kForbiddenMarkerColor);
                }

                for (const auto keyCell : document.map.markers.positionsOf(map::Marker::Key))
                {
                    ruled(voxelmap::getCubeWireframe(keyCell), kRuleLineColor);
                }

                for (
                    const auto doorCell : document.map.markers.positionsOf(map::Marker::Door))
                {
                    ruled(
                        voxelmap::getCubeWireframe(doorCell),
                        kCornerSeamLineColor);
                }

                for (const auto checkpointCell :
                     document.map.markers.positionsOf(map::Marker::Checkpoint))
                {
                    ruled(
                        voxelmap::getCubeWireframe(checkpointCell),
                        kRuleLineCrossLevelColor);
                }

                for (const auto foodCell : document.map.markers.positionsOf(map::Marker::Food))
                {
                    ruled(voxelmap::getCubeWireframe(foodCell), kFoodBarColor);
                }

                for (const auto waterCell : document.map.markers.positionsOf(map::Marker::Water))
                {
                    ruled(voxelmap::getCubeWireframe(waterCell), kWaterBarColor);
                }

                if (preferences.tool == map::Tool::Figure
                    && figureTool.chosenIndex.has_value()
                    && *figureTool.chosenIndex < document.map.characters.size())
                {
                    for (const auto stop :
                         document.map.characters.at(
                             *figureTool.chosenIndex).patrolPathPositions)
                    {
                        ruled(
                            voxelmap::getCubeWireframe(stop),
                            kPlacementPreviewColor);
                    }
                }

                if (preferences.tool == map::Tool::PressurePlate)
                {
                    for (std::size_t index = 0;
                         index < document.map.plates.size();
                         ++index)
                    {
                        ruled(
                            voxelmap::getCubeWireframe(document.map.plates.at(
                                index).position),
                            kCursorColor);

                        if (plateTool.chosenIndex == index)
                        {
                            for (const auto sway :
                                 document.map.plates.at(index).togglePositions)
                            {
                                ruled(
                                    voxelmap::getCubeWireframe(sway),
                                    kPlacementPreviewColor);
                            }
                        }
                    }
                }
            }
        }

        const auto seamFrom = clockSource.getCurrentTime();

        if (preferences.showRuleLines && !play.playing)
        {
            const auto &faces = worldMeshes.getFaces();
            const auto seamRuled =
                [&](const std::vector<solver::FaceSeam> &seams,
                    const gfx::Color ink)
            {
                for (const auto &seam : seams)
                {
                    if (!voxelmap::isFrontFacing(
                            camera,
                            modelMatrix,
                            faces[seam.faceA].side)
                        || !voxelmap::isFrontFacing(
                            camera,
                            modelMatrix,
                            faces[seam.faceB].side))
                    {
                        continue;
                    }

                    const auto herePoint = voxelmap::getProjectToScreen(
                        clipMatrix,
                        camera::kCanvasSize,
                        voxelmap::getFaceMiddle(faces[seam.faceA]));
                    const auto therePoint = voxelmap::getProjectToScreen(
                        clipMatrix,
                        camera::kCanvasSize,
                        voxelmap::getFaceMiddle(faces[seam.faceB]));

                    if (herePoint.has_value()
                        && therePoint.has_value())
                    {
                        viewportRenderer.drawLine(
                            *herePoint,
                            *therePoint,
                            solver::isCornerSeam(faces, seam)
                                ? kCornerSeamLineColor
                                : ink);
                    }
                }
            };

            seamRuled(overlays.seamsAboveLevel, kRuleLineCrossLevelColor);
            seamRuled(overlays.seamsAtLevel, kRuleLineColor);
        }

        meters.seamRate.record(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                clockSource.getCurrentTime() - seamFrom));

        drawHealthBars(viewContext, clipMatrix);
        drawSightPoints(viewContext, clipMatrix);
    }

    void WorldView::drawPointMark(
        const ViewContext &viewContext,
        const gfx::Mat4 &clipMatrix,
        const gfx::Vec3 position,
        const gfx::Color markColor)
    {
        auto &viewportRenderer = viewContext.render.viewportRenderer;

        const auto onCanvas = voxelmap::getProjectToScreen(
            clipMatrix, camera::kCanvasSize, position);

        if (!onCanvas.has_value())
        {
            return;
        }

        viewportRenderer.drawRect(
            gfx::RectF{
                gfx::PointF{
                    onCanvas->x - (kSightPointSide / 2.0F),
                    onCanvas->y - (kSightPointSide / 2.0F)},
                gfx::SizeF{kSightPointSide, kSightPointSide}},
            markColor);
    }

    void WorldView::drawSightPoints(
        const ViewContext &viewContext,
        const gfx::Mat4 &clipMatrix)
    {
        auto &play = viewContext.play;

        drawPointMark(viewContext, clipMatrix, gfx::Vec3{0.0F, 0.0F, 0.0F}, kOriginPointColor);

        if (!play.playing)
        {
            return;
        }

        if (!worldEdit.lowerSight)
        {
            return;
        }

        const auto stoodPosition =
            play.world.get<component::Position>(play.game->getPlayer());

        const gfx::Vec3 walkerPosition{
            stoodPosition.x, stoodPosition.y, stoodPosition.z};

        drawPointMark(viewContext, clipMatrix,
            antwika::voxel::getLineOfSight(walkerPosition),
            kSightPointColor);

        if (isUpperSightOn(viewContext))
        {
            drawPointMark(viewContext, clipMatrix,
                antwika::voxel::getUpperLineOfSight(walkerPosition),
                kSightPointColor);
        }
    }

    void WorldView::drawHealthBars(
        const ViewContext &viewContext,
        const gfx::Mat4 &clipMatrix)
    {
        auto &play = viewContext.play;
        auto &viewportRenderer = viewContext.render.viewportRenderer;

        if (!play.playing)
        {
            return;
        }

        static constexpr std::array<gfx::Color,
            antwika::render::kHealthBarParts>
            kBarColors{
                kHealthBarEmptyColor,
                kFoodBarColor,
                kHealthBarEmptyColor,
                kWaterBarColor};

        for (const auto entity :
             play.game->getWorld().view<component::Position, component::Health>())
        {
            const auto overhead = voxelmap::getProjectToScreen(
                clipMatrix,
                camera::kCanvasSize,
                character::headTopOf(
                    play.game->getWorld().get<component::Position>(entity)));

            if (!overhead.has_value())
            {
                continue;
            }

            const auto bars = antwika::render::getHealthBars(
                *overhead, play.game->getWorld().get<component::Health>(entity));

            for (std::size_t index = 0;
                 index < antwika::render::kHealthBarParts;
                 ++index)
            {
                viewportRenderer.drawRect(bars.at(index), kBarColors.at(index));
            }
        }
    }

}
