#include <antwika/component/Health.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/render/HealthBars.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

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

    void Editor::drawWorldOverlays(
        const ui::Frame &frame,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix)
    {
        if (!play.playing && overlayStale)
        {
            gridLines = voxelmap::getLevelGridLines(
                document.map.voxels, antwika::voxel::getCubeTop(editLevel));
            topLines = voxelmap::getBuildableTopOutlines(
                document.map.voxels, antwika::voxel::getCubeTop(editLevel));

            const auto satisfiedSeamSet = solver::getSatisfiedSeams(
                worldMeshes.getFaces(),
                worldMeshes.getSolved(),
                document.map.rules,
                cornerJoining);

            seamsAboveLevel = solver::getCrossLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(editLevel));
            seamsAtLevel = solver::getSameLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(editLevel));
            overlayStale = false;
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
                for (const auto keyCell : document.map.keyPositions)
                {
                    if (!play.game->getGates().collectedKeyPositions.contains(
                            antwika::voxel::cubeCornerOf(keyCell)))
                    {
                        ruled(voxelmap::getCubeWireframe(keyCell), kRuleLineColor);
                    }
                }

                for (const auto doorCell : document.map.doorPositions)
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

            for (const auto troubleCell : growTroublePositions)
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
                if (!lightPasses.getHidden().empty())
                {
                    ruled(
                        voxelmap::getOccluderFootprints(
                            lightPasses.getHidden()),
                        kLevelGridLineColor);
                }

                if (!cameraRig.freeLook && settings.grid)
                {
                    ruled(gridLines, kLevelGridLineColor);
                }

                if (!cameraRig.freeLook)
                {
                    ruled(topLines, kCursorColor);
                }

                const auto going = voxelmap::getCellUnder(
                    camera,
                    modelMatrix,
                    camera::kCanvasSize,
                    pointer.pointerOnCanvas,
                    antwika::voxel::getCubeTop(editLevel));

                const auto steering =
                    cameraRig.orbiting
                    || (cameraRig.panning
                        && cameraRig.panGripPosition.has_value())
                    || play.game->wasdKeys() != input::DirectionKeys{}
                    || play.game->arrowKeys() != input::DirectionKeys{}
                    || descendHeld
                    || ascendHeld;

                if (going.has_value() && !cameraRig.freeLook
                    && settings.showPlacementGhost && !steering
                    && !frame.interactions.pointerOverUi)
                {
                    if (settings.tool == map::Tool::Stamp
                        && (stampFromPosition.has_value()
                            || !stampVoxels.empty()))
                    {
                        for (const auto cube :
                             getStampGhost(*going))
                        {
                            ruled(
                                voxelmap::getCubeWireframe(cube),
                                    kPlacementPreviewColor);
                        }
                    }
                    else if (shapeFromPosition.has_value())
                    {
                        for (const auto cube : getShapedCubes(
                                 *shapeFromPosition, *going))
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

                for (const auto keyCell : document.map.keyPositions)
                {
                    ruled(voxelmap::getCubeWireframe(keyCell), kRuleLineColor);
                }

                for (
                    const auto doorCell : document.map.doorPositions)
                {
                    ruled(
                        voxelmap::getCubeWireframe(doorCell),
                        kCornerSeamLineColor);
                }

                for (const auto checkpointCell :
                     document.map.checkpointPositions)
                {
                    ruled(
                        voxelmap::getCubeWireframe(checkpointCell),
                        kRuleLineCrossLevelColor);
                }

                for (const auto foodCell : document.map.foodPositions)
                {
                    ruled(voxelmap::getCubeWireframe(foodCell), kFoodBarColor);
                }

                for (const auto waterCell : document.map.waterPositions)
                {
                    ruled(voxelmap::getCubeWireframe(waterCell), kWaterBarColor);
                }

                if (settings.tool == map::Tool::Figure
                    && figurePicked.has_value()
                    && *figurePicked < document.map.characters.size())
                {
                    for (const auto stop :
                         document.map.characters.at(
                             *figurePicked).patrolPathPositions)
                    {
                        ruled(
                            voxelmap::getCubeWireframe(stop),
                            kPlacementPreviewColor);
                    }
                }

                if (settings.tool == map::Tool::PressurePlate)
                {
                    for (std::size_t index = 0;
                         index < document.map.plates.size();
                         ++index)
                    {
                        ruled(
                            voxelmap::getCubeWireframe(document.map.plates.at(
                                index).position),
                            kCursorColor);

                        if (platePicked == index)
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

        if (settings.showRuleLines && !play.playing)
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

            seamRuled(seamsAboveLevel, kRuleLineCrossLevelColor);
            seamRuled(seamsAtLevel, kRuleLineColor);
        }

        meters.seamRate.record(
            std::chrono::duration_cast<
                std::chrono::nanoseconds>(
                clockSource.getCurrentTime() - seamFrom));

        drawHealthBars(clipMatrix);
        drawSightPoints(clipMatrix);
    }

    void Editor::drawPointMark(
        const gfx::Mat4 &clipMatrix,
        const gfx::Vec3 position,
        const gfx::Color markColor)
    {
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

    void Editor::drawSightPoints(const gfx::Mat4 &clipMatrix)
    {
        drawPointMark(
            clipMatrix, gfx::Vec3{0.0F, 0.0F, 0.0F}, kOriginPointColor);

        if (!play.playing)
        {
            return;
        }

        if (!lowerSight)
        {
            return;
        }

        const auto stoodPosition =
            play.world.get<component::Position>(play.game->getPlayer());

        const gfx::Vec3 walkerPosition{
            stoodPosition.x, stoodPosition.y, stoodPosition.z};

        drawPointMark(
            clipMatrix,
            antwika::voxel::getLineOfSight(walkerPosition),
            kSightPointColor);

        if (upperSightOn())
        {
            drawPointMark(
                clipMatrix,
                antwika::voxel::getUpperLineOfSight(walkerPosition),
                kSightPointColor);
        }
    }

    void Editor::drawHealthBars(const gfx::Mat4 &clipMatrix)
    {
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
