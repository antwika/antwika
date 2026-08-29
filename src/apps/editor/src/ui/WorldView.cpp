#include "antwika/editor/ui/WorldView.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshBox.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/Viewport.hpp>
#include <antwika/input/DirectionKeys.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/render/HealthBars.hpp>
#include <antwika/solver/VoxelWeave.hpp>
#include <antwika/text/TextLayout.hpp>
#include <antwika/time/FrameRate.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/WorldCamera.hpp"
#include "antwika/editor/tools/ShapedCubes.hpp"
#include "antwika/editor/ui/GizmoSheet.hpp"
#include "antwika/editor/ui/IconSheet.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"
#include "antwika/editor/view/WorldSprites.hpp"

namespace antwika::editor
{

    namespace
    {

        /**
         * @brief The canvas the world is drawn on, which is the world
         * panel while the editor is up and the whole canvas otherwise.
         */
        [[nodiscard]] gfx::RectF getWorldClipRect(
            const gfx::Viewport viewport, const ui::Frame &frame)
        {
            const auto panelRect =
                frame.rects.getWidgetRect(kWorldPanelWidget);

            if (!panelRect.has_value())
            {
                return gfx::RectF(
                    {0.0F, 0.0F},
                    {static_cast<float>(camera::kCanvasSize.width),
                     static_cast<float>(camera::kCanvasSize.height)});
            }

            return viewport.toCanvas(
                gfx::RectF(
                    {static_cast<float>(panelRect->originPoint.x),
                     static_cast<float>(panelRect->originPoint.y)},
                    {static_cast<float>(panelRect->size.width),
                     static_cast<float>(panelRect->size.height)}));
        }

        struct ToolStatusRow final
        {
            Tool tool;
            std::string_view status;
        };

        constexpr std::array<ToolStatusRow, enums::kCount<Tool>>
            kToolStatuses{{
            {Tool::Select,
             "1 world - lmb picks an entity - drag moves it - "
             "rmb lets go"},
            {Tool::Brush,
             "1 world - lmb adds - rmb takes - f5 plays"},
            {Tool::Picker,
             "1 world - lmb picks a tile - rmb takes"},
            {Tool::Lamp,
             "1 world - lmb sets a lamp of the ink chosen - rmb takes"},
            {Tool::Start,
             "1 world - lmb sets the start cube - rmb takes it"},
            {Tool::Exit,
             "1 world - lmb sets the exit cube - rmb takes it"},
            {Tool::Stamp,
             "1 world - drag copies cubes - lmb sets them down - rmb "
             "drops them"},
            {Tool::Character,
             "1 world - lmb stands the chosen character here, again "
             "adds a stop - rmb takes it away"},
            {Tool::Checkpoint,
             "1 world - lmb picks a tile - rmb takes"},
            {Tool::Food,
             "1 world - lmb lays food to pick up - rmb takes it"},
            {Tool::Water,
             "1 world - lmb lays water to pick up - rmb takes it"},
            {Tool::Eraser,
             "1 world - lmb clears cubes - drag sweeps them away"}}};

        static_assert(
            enums::tagsInOrder(kToolStatuses, &ToolStatusRow::tool));

        /**
         * @brief How wide a gizmo icon stands in the world, in voxels.
         */
        constexpr float kGizmoIconVoxels = 1.0F;

        /**
         * @brief How long a world span reads on the canvas about a point.
         * The three world axes fall onto the canvas' two, so their squared
         * canvas lengths add up to twice the span's own, whichever way the
         * camera faces.
         */
        [[nodiscard]] std::optional<float> getCanvasSpan(
            const gfx::Mat4 &clipMatrix,
            const gfx::Vec3 middlePoint,
            const float worldSpan)
        {
            const auto point = voxelmap::getProjectToScreen(
                clipMatrix, camera::kCanvasSize, middlePoint);

            if (!point.has_value())
            {
                return std::nullopt;
            }

            const std::array<gfx::Vec3, 3> axisOffsets{
                gfx::Vec3{worldSpan, 0.0F, 0.0F},
                gfx::Vec3{0.0F, worldSpan, 0.0F},
                gfx::Vec3{0.0F, 0.0F, worldSpan}};

            auto squaredSum = 0.0F;

            for (const auto axisOffset : axisOffsets)
            {
                const auto alongPoint = voxelmap::getProjectToScreen(
                    clipMatrix,
                    camera::kCanvasSize,
                    middlePoint + axisOffset);

                if (!alongPoint.has_value())
                {
                    return std::nullopt;
                }

                const auto acrossCanvas = alongPoint->x - point->x;
                const auto downCanvas = alongPoint->y - point->y;

                squaredSum += (acrossCanvas * acrossCanvas)
                              + (downCanvas * downCanvas);
            }

            return std::sqrt(squaredSum / 2.0F);
        }

    }

    bool WorldView::claims(
        const View shownView, const bool playing) const noexcept
    {
        return playing || shownView == View::World;
    }

    bool WorldView::offersPaint(const Paint) const noexcept
    {
        return false;
    }

    std::string WorldView::getStatusText(
        const ViewContext &viewContext) const
    {
        const auto &statusRow = enums::lookup(
            kToolStatuses, viewContext.workbench.preferences.tool);

        return std::string(statusRow.status)
               + " - wheel zooms - g - c "
               + std::string(
                   worldEditState.getCornerJoining()
                           == solver::CornerSeams::Included
                            ? "on"
                            : "off")
               + " - level " + std::to_string(worldEditState.getEditLevel());
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
        auto &characterSkins = viewContext.render.characterSkins;
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
        const auto worldScope = viewportRenderer.clipScope(
            getWorldClipRect(viewportRenderer.getViewport(), frame));

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
                                 component::CharacterIndex>())
                    {
                        const auto index =
                            play.game->getWorld()
                                .get<component::CharacterIndex>(entity)
                                .index;

                        if (index >= characterSkins.getSheets().size())
                        {
                            continue;
                        }

                        drawSprite(
                            viewContext.render,
                            tick,
                            camera,
                            modelMatrix,
                            characterSkins.getPicture(index),
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
                gfx::TextScale{.multiplier = 1},
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
                gfx::TextScale{.multiplier = 1},
                kTextColor);
            viewportRenderer.drawText(
                {camera::kCanvasSize.width / 2.0F - 60.0F,
                 camera::kCanvasSize.height / 2.0F + 4.0F},
                "press any key",
                gfx::TextScale{.multiplier = 1},
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
                         antwika::text::getTextSize(
                             rateText, gfx::TextScale{.multiplier = 1})
                             .width)
                     - 4.0F,
                 4.0F},
                rateText,
                gfx::TextScale{.multiplier = 1},
                kTextColor);
            viewportRenderer.drawText(
                {static_cast<float>(camera::kCanvasSize.width)
                     - static_cast<float>(
                         antwika::text::getTextSize(
                             workText, gfx::TextScale{.multiplier = 1})
                             .width)
                     - 4.0F,
                 16.0F},
                workText,
                gfx::TextScale{.multiplier = 1},
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
                document.map.voxels, antwika::voxel::getCubeTop(worldEditState.getEditLevel()));
            overlays.topLines = voxelmap::getBuildableTopOutlines(
                document.map.voxels, antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

            const auto satisfiedSeamSet = solver::getSatisfiedSeams(
                worldMeshes.getFaces(),
                worldMeshes.getSolvedTiles(),
                document.map.rules,
                worldEditState.getCornerJoining());

            overlays.seamsAboveLevel = solver::getCrossLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(worldEditState.getEditLevel()));
            overlays.seamsAtLevel = solver::getSameLevelSeams(
                worldMeshes.getFaces(),
                satisfiedSeamSet,
                antwika::voxel::getCubeTop(worldEditState.getEditLevel()));
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

            const auto &gizmos = viewContext.workbench.gizmos;
            const auto markedAt = [&](const gfx::Vec3 middlePoint,
                                      const GizmoKind kind)
            {
                const auto slot = enums::index(kind);

                if (gizmos.texture == nullptr
                    || !isGizmoDrawn(gizmos.sheetBitmap, slot))
                {
                    return false;
                }

                const auto point = voxelmap::getProjectToScreen(
                    clipMatrix, camera::kCanvasSize, middlePoint);

                if (!point.has_value())
                {
                    return false;
                }

                const auto iconSide = getCanvasSpan(
                    clipMatrix,
                    middlePoint,
                    kGizmoIconVoxels * voxel::kVoxelSide);

                if (!iconSide.has_value())
                {
                    return false;
                }

                viewportRenderer.drawTexture(
                    *gizmos.texture,
                    getIconSource(slot),
                    antwika::gfx::RectF(
                        {point->x - (*iconSide / 2.0F),
                         point->y - (*iconSide / 2.0F)},
                        {*iconSide, *iconSide}),
                    kWhiteColor);

                return true;
            };

            const auto marked = [&markedAt](
                                    const voxel::VoxelPosition gizmoCell,
                                    const GizmoKind kind)
            {
                return markedAt(voxelmap::getCubeMiddle(gizmoCell), kind);
            };

            if (play.playing)
            {
                for (const auto entity :
                     play.game->getWorld().view<antwika::component::Item>())
                {
                    const auto item =
                        play.game->getWorld().get<antwika::component::Item>(
                            entity);
                    const auto itemGizmo =
                        static_cast<component::ItemKind>(item.kind)
                                == component::ItemKind::Food
                            ? GizmoKind::Food
                            : GizmoKind::Water;

                    if (!marked(item.position, itemGizmo))
                    {
                        ruled(
                            voxelmap::getCubeGizmoSpans(item.position),
                            kWhiteColor);
                    }
                }
            }

            if (play.playing && play.game->getPathGoal().has_value())
            {
                ruled(
                    voxelmap::getCubeWireframe(*play.game->getPathGoal()),
                    kPlacementPreviewColor);
            }

            if (!play.playing)
            {
                for (const auto troubleCell : grow.troublePositions)
                {
                    ruled(
                        voxelmap::getCubeWireframe(troubleCell),
                        kForbiddenMarkerColor);
                }

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
                    antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

                const auto steering =
                    cameraRig.orbiting
                    || (cameraRig.panning
                        && cameraRig.panGripPosition.has_value())
                    || play.wasdKeys != input::DirectionKeys{}
                    || play.arrowKeys != input::DirectionKeys{}
                    || worldEditState.isRiseHeld();

                if (going.has_value() && !cameraRig.freeLook
                    && preferences.showPlacementGhost && !steering
                    && !frame.interactions.pointerOverUi)
                {
                    if (preferences.tool == Tool::Stamp
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

                if (preferences.tool == Tool::Select
                    && viewContext.workbench.entityPick.kind.has_value())
                {
                    ruled(
                        voxelmap::getCubeWireframe(
                            viewContext.workbench.entityPick.position),
                        kSelectionAccentColor);
                }

                for (const auto lamp : document.map.lamps)
                {
                    if (!markedAt(
                            light::getLampPosition(lamp), GizmoKind::Lamp))
                    {
                        ruled(light::getLampGizmoSpans(lamp), kWhiteColor);
                    }
                }

                if (document.map.spawnCubePosition.has_value()
                    && !marked(
                        *document.map.spawnCubePosition, GizmoKind::Spawn))
                {
                    ruled(
                        voxelmap::getCubeGizmoSpans(
                            *document.map.spawnCubePosition),
                        kWhiteColor);
                }

                if (document.map.exitCubePosition.has_value()
                    && !marked(
                        *document.map.exitCubePosition, GizmoKind::Exit))
                {
                    ruled(
                        voxelmap::getCubeGizmoSpans(
                            *document.map.exitCubePosition),
                        kWhiteColor);
                }

                for (const auto checkpointCell :
                     document.map.markers.positionsOf(map::Marker::Checkpoint))
                {
                    if (!marked(checkpointCell, GizmoKind::Checkpoint))
                    {
                        ruled(
                            voxelmap::getCubeGizmoSpans(checkpointCell),
                            kWhiteColor);
                    }
                }

                for (const auto foodCell : document.map.markers.positionsOf(map::Marker::Food))
                {
                    if (!marked(foodCell, GizmoKind::Food))
                    {
                        ruled(
                            voxelmap::getCubeGizmoSpans(foodCell),
                            kWhiteColor);
                    }
                }

                for (const auto waterCell : document.map.markers.positionsOf(map::Marker::Water))
                {
                    if (!marked(waterCell, GizmoKind::Water))
                    {
                        ruled(
                            voxelmap::getCubeGizmoSpans(waterCell),
                            kWhiteColor);
                    }
                }

                if (const auto chosenCharacter = characterToolState.getChosenCharacter(
                        document.map.characters.size());
                    preferences.tool == Tool::Character
                    && chosenCharacter.has_value())
                {
                    for (const auto stop :
                         document.map.characters.at(
                             *chosenCharacter).patrolPathPositions)
                    {
                        ruled(
                            voxelmap::getCubeWireframe(stop),
                            kPlacementPreviewColor);
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

    void WorldView::trackPointer(const ViewContext &viewContext)
    {
        carryLamp(viewContext);

        auto &pointer = viewContext.workbench.pointer;
        auto &preferences = viewContext.workbench.preferences;
        auto &drawnMap = viewContext.document.map;

        if (worldPaint.dragButton.has_value()
            && !worldPaint.shapeFromPosition.has_value())
        {
            const auto cell = voxelmap::getCellUnder(
                getWorldCamera(viewContext.play, viewContext.cameraRig),
                getWorldRotation(viewContext.play),
                camera::kCanvasSize,
                pointer.pointerOnCanvas,
                antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

            if (cell.has_value() && cell != worldPaint.lastPaintedPosition)
            {
                drawnMap.voxels = voxel::getWithRampsRebuilt(
                    preferences.tool == Tool::Eraser
                          ? voxel::withoutBlockAt(
                              drawnMap.voxels, *cell)
                        : voxel::withBlockAt(
                              drawnMap.voxels,
                              *cell,
                              preferences.kind,
                              voxel::Facing::Any),
                    *cell);
                worldPaint.lastPaintedPosition = cell;
                viewContext.workbench.remesh.pending = true;
            }
        }
    }

    void WorldView::carryLamp(const ViewContext &viewContext)
    {
        if (!worldPaint.draggedLamp.has_value())
        {
            return;
        }

        auto &drawnMap = viewContext.document.map;
        const auto position = voxelmap::getCellUnder(
            getWorldCamera(viewContext.play, viewContext.cameraRig),
            getWorldRotation(viewContext.play),
            camera::kCanvasSize,
            viewContext.workbench.pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

        const auto cornerPosition = position.has_value()
                                        ? antwika::voxel::cubeCornerOf(
                                              *position)
                                        : voxel::VoxelPosition{};

        if (position.has_value()
            && cornerPosition != worldPaint.draggedLamp->position)
        {
            drawnMap.lamps = light::withLampAt(
                light::withoutLampAt(
                    drawnMap.lamps,
                    worldPaint.draggedLamp->position),
                cornerPosition,
                worldPaint.draggedLamp->tintColor);
            worldPaint.draggedLamp->position = cornerPosition;
            viewContext.render.lightPasses.forget();
        }
    }

    bool WorldView::beginShape(
        const ViewContext &viewContext,
        const voxel::VoxelPosition position,
        const input::MouseButton button)
    {
        auto &preferences = viewContext.workbench.preferences;

        if (preferences.tool != Tool::Brush
            || (preferences.paint != Paint::Rect
                && preferences.paint != Paint::Line))
        {
            return false;
        }

        worldPaint.shapeFromPosition = position;
        worldPaint.dragButton = button;

        return true;
    }

    void WorldView::finishShape(
        const ViewContext &viewContext, const input::MouseButton button)
    {
        if (!worldPaint.shapeFromPosition.has_value()
            || !worldPaint.dragButton.has_value()
            || button != *worldPaint.dragButton)
        {
            return;
        }

        auto &preferences = viewContext.workbench.preferences;
        auto &drawnMap = viewContext.document.map;
        const auto position = voxelmap::getCellUnder(
            getWorldCamera(viewContext.play, viewContext.cameraRig),
            getWorldRotation(viewContext.play),
            camera::kCanvasSize,
            viewContext.workbench.pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

        if (position.has_value())
        {
            viewContext.editSteps.pushUndo();

            for (const auto cube :
                 getShapedCubes(
                     *worldPaint.shapeFromPosition,
                     *position,
                     preferences.paint))
            {
                drawnMap.voxels = voxel::getWithRampsRebuilt(
                    worldPaint.dragButton == input::MouseButton::Left
                                     ? voxel::withBlockAt(
                              drawnMap.voxels,
                              cube,
                              preferences.kind,
                              voxel::Facing::Any)
                        : voxel::withoutBlockAt(
                              drawnMap.voxels, cube),
                    cube);
            }

            viewContext.editSteps.rebuildWorld();
        }

        worldPaint.shapeFromPosition.reset();
    }

    void WorldView::pressStamp(
        const ViewContext &viewContext,
        const voxel::VoxelPosition position,
        const input::MouseButton button)
    {
        auto &drawnMap = viewContext.document.map;

        if (button == input::MouseButton::Right)
        {
            stamp.voxels.clear();
            stamp.fromPosition.reset();

            return;
        }

        if (!stamp.voxels.empty())
        {
            viewContext.editSteps.pushUndo();

            const auto stampCorner = antwika::voxel::cubeCornerOf(position);

            for (const auto &[offset, material] : stamp.voxels)
            {
                const voxel::VoxelPosition cornerPosition{
                    .x = stampCorner.x + offset.x,
                    .y = offset.y,
                    .z = stampCorner.z + offset.z};

                drawnMap.voxels = voxel::getWithRampsRebuilt(
                    voxel::withBlockAt(
                        drawnMap.voxels,
                        cornerPosition,
                        material.kind,
                        material.facing),
                    cornerPosition);
            }

            viewContext.editSteps.rebuildWorld();

            return;
        }

        stamp.fromPosition = position;
    }

    void WorldView::finishStamp(
        const ViewContext &viewContext, const input::MouseButton button)
    {
        if (!stamp.fromPosition.has_value()
            || button != input::MouseButton::Left)
        {
            return;
        }

        const auto position = voxelmap::getCellUnder(
            getWorldCamera(viewContext.play, viewContext.cameraRig),
            getWorldRotation(viewContext.play),
            camera::kCanvasSize,
            viewContext.workbench.pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(worldEditState.getEditLevel()));

        if (!position.has_value())
        {
            stamp.fromPosition.reset();

            return;
        }

        const auto a = antwika::voxel::cubeCornerOf(*stamp.fromPosition);
        const auto b = antwika::voxel::cubeCornerOf(*position);
        const auto lowX = std::min(a.x, b.x);
        const auto highX = std::max(a.x, b.x);
        const auto lowZ = std::min(a.z, b.z);
        const auto highZ = std::max(a.z, b.z);

        voxel::Voxels cubeVoxels;

        for (const auto &[cubePosition, material] : viewContext.document.map.voxels)
        {
            const auto corner =
                antwika::voxel::cubeCornerOf(cubePosition);

            if (corner.x < lowX || corner.x > highX
                || corner.z < lowZ || corner.z > highZ)
            {
                continue;
            }

            cubeVoxels.emplace(corner, material);
        }

        stamp.voxels.clear();

        for (const auto &[corner, sample] : cubeVoxels)
        {
            stamp.voxels[voxel::VoxelPosition{
                .x = corner.x - lowX,
                .y = corner.y,
                .z = corner.z - lowZ}] = sample;
        }

        stamp.fromPosition.reset();
    }

    bool WorldView::beginLampCarry(
        const ViewContext &viewContext, const voxel::VoxelPosition position)
    {
        for (const auto &lamp : viewContext.document.map.lamps)
        {
            if (lamp.position == position)
            {
                viewContext.editSteps.pushUndo();
                worldPaint.draggedLamp = lamp;

                return true;
            }
        }

        return false;
    }

    void WorldView::beginPaintDrag(
        const voxel::VoxelPosition position,
        const input::MouseButton button) noexcept
    {
        worldPaint.dragButton = button;
        worldPaint.lastPaintedPosition = position;
    }

    void WorldView::endPaintDrag(const input::MouseButton button) noexcept
    {
        if (worldPaint.draggedLamp.has_value()
            && button == input::MouseButton::Left)
        {
            worldPaint.draggedLamp.reset();
        }

        if (worldPaint.dragButton.has_value()
            && button == *worldPaint.dragButton)
        {
            worldPaint.dragButton.reset();
            worldPaint.lastPaintedPosition.reset();
        }
    }

    void WorldView::endDrags() noexcept
    {
        worldPaint.shapeFromPosition.reset();
        worldPaint.dragButton.reset();
        stamp.voxels.clear();
        stamp.fromPosition.reset();
    }

    void WorldView::markOverlaysStale() noexcept
    {
        overlays.stale = true;
    }

    std::uint64_t WorldView::takeGrowSeed() noexcept
    {
        return grow.seed++;
    }

    void WorldView::setGrowTrouble(
        std::vector<voxel::VoxelPosition> troublePositions)
    {
        grow.troublePositions = std::move(troublePositions);
    }

    void WorldView::clearGrowTrouble() noexcept
    {
        grow.troublePositions.clear();
    }

    WorldEdit &WorldView::worldEdit() noexcept
    {
        return worldEditState;
    }

    CharacterTool &WorldView::characterTool() noexcept
    {
        return characterToolState;
    }

    const CharacterTool &WorldView::getCharacterTool() const noexcept
    {
        return characterToolState;
    }

}
