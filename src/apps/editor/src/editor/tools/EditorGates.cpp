#include <antwika/gameplay/GateState.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/ConsumeIntent.hpp>
#include <antwika/component/ConsumeReport.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::resetGates()
    {
        play.game->gates() = gameplay::GateState{};
    }

    void Editor::clearAssignModes()
    {
        assignMode = AssignMode{};
    }

    void Editor::pressGate(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        const auto chosenTool = settings.tool;

        auto &drawnMap = document.map;

        auto &gateCells =
            chosenTool == map::Tool::Key     ? drawnMap.keyPositions
            : chosenTool == map::Tool::Door  ? drawnMap.doorPositions
            : chosenTool == map::Tool::Food  ? drawnMap.foodPositions
            : chosenTool == map::Tool::Water ? drawnMap.waterPositions
                                             : drawnMap.checkpointPositions;
        const auto foundCube = rules::gateCubeContaining(gateCells, position);

        if (button == input::MouseButton::Right)
        {
            if (!foundCube.has_value())
            {
                return;
            }

            pushUndo();
            std::erase_if(
                gateCells,
                [this, corner = *foundCube](const voxel::VoxelPosition one)
                {
                    const auto gone =
                        antwika::voxel::cubeCornerOf(one) == corner;

                    if (gone && settings.tool == map::Tool::Door)
                    {
                        document.map.voxels = voxel::withRampsRebuilt(
                            voxel::withoutBlockAt(document.map.voxels, one),
                            one);
                    }

                    return gone;
                });

            if (settings.tool == map::Tool::Door)
            {
                rebuildWorld();
            }

            return;
        }

        if (foundCube.has_value())
        {
            return;
        }

        pushUndo();
        gateCells.push_back(position);

        if (!rules::cubeOccupied(document.map.voxels,
            antwika::voxel::cubeCornerOf(position))
            || settings.tool == map::Tool::Door)
        {
            document.map.voxels = voxel::withRampsRebuilt(
                voxel::withBlockAt(document.map.voxels, position), position);
            rebuildWorld();
        }
    }

    void Editor::onSteppedWorld(const gfx::Vec3 walkerPosition)
    {
        const voxel::VoxelPosition standsInPosition{
            .x = static_cast<std::int32_t>(
                std::floor(walkerPosition.x / voxel::kVoxelSide)),
            .y = static_cast<std::int32_t>(
                std::floor(walkerPosition.y / voxel::kVoxelSide)),
            .z = static_cast<std::int32_t>(
                std::floor(walkerPosition.z / voxel::kVoxelSide))};
        const voxel::VoxelPosition standsOnPosition{
            .x = standsInPosition.x,
            .y = standsInPosition.y - 1,
            .z = standsInPosition.z};

        onSteppedPlates(standsOnPosition);
        onSteppedGates(standsInPosition, standsOnPosition);

        if (document.map.exitCubePosition.has_value()
            && (antwika::voxel::cubeCornerOf(standsInPosition)
                    == antwika::voxel::cubeCornerOf(
                        *document.map.exitCubePosition)
                || antwika::voxel::cubeCornerOf(standsOnPosition)
                       == antwika::voxel::cubeCornerOf(
                           *document.map.exitCubePosition)))
        {
            takeExit();
        }
    }

    void Editor::onSteppedGates(
        const voxel::VoxelPosition standsInPosition,
        const voxel::VoxelPosition standsOnPosition)
    {
        if (play.game->gates().lockedExitAnnouncedPosition.has_value()
            && (!document.map.exitCubePosition.has_value()
                || (antwika::voxel::cubeCornerOf(standsInPosition)
                        != antwika::voxel::cubeCornerOf(
                            *document.map.exitCubePosition)
                    && antwika::voxel::cubeCornerOf(standsOnPosition)
                           != antwika::voxel::cubeCornerOf(
                               *document.map.exitCubePosition))))
        {
            play.game->gates().lockedExitAnnouncedPosition.reset();
        }

        onSteppedKeys(standsInPosition, standsOnPosition);
        onSteppedCheckpoints(standsOnPosition);
        onSteppedDoors(standsInPosition);
    }

    void Editor::onSteppedKeys(
        const voxel::VoxelPosition standsInPosition,
        const voxel::VoxelPosition standsOnPosition)
    {
        for (const auto gateCell : {standsInPosition, standsOnPosition})
        {
            const auto foundCube = rules::gateCubeContaining(
                document.map.keyPositions,
            gateCell);

            if (!foundCube.has_value()
                || play.game->gates().collectedKeyPositions.contains(
                    *foundCube))
            {
                continue;
            }

            play.game->gates().collectedKeyPositions.insert(*foundCube);
            play.game->gates().keysHeld += 1;
            sayCaption(
                "a key",
                "taken (" + std::to_string(play.game->gates().keysHeld)
                    + " held)");
        }

    }

    void Editor::consumeItem(const component::ItemKind kind)
    {
        auto &gameWorld = play.game->world();
        const ecs::OpenPhase phase(gameWorld);

        gameWorld.add<component::ConsumeIntent>(
            play.game->player(),
            component::ConsumeIntent{
                .kind = static_cast<std::uint8_t>(kind)});
    }

    void Editor::sayConsumeReport()
    {
        auto &gameWorld = play.game->world();

        if (!gameWorld.has<component::ConsumeReport>(play.game->player()))
        {
            return;
        }

        const auto report =
            gameWorld.get<component::ConsumeReport>(play.game->player());
        const auto kind = static_cast<component::ItemKind>(report.kind);

        sayCaption(
            kind == component::ItemKind::Food ? "food" : "water",
            report.anyLeft
                ? (kind == component::ItemKind::Food ? "eaten" : "drunk")
                : "there is none left to take");

        const ecs::OpenPhase phase(gameWorld);

        gameWorld.remove<component::ConsumeReport>(play.game->player());
    }

    void Editor::onSteppedCheckpoints(
        const voxel::VoxelPosition standsOnPosition)
    {
        const auto pad =
            rules::gateCubeContaining(document.map.checkpointPositions,
                standsOnPosition);

        if (pad.has_value() && play.game->gates().checkpointOnPosition != pad)
        {
            const auto stoodPosition =
                play.game->world().get<component::Position>(play.game->player(
                        ));

            play.game->gates().checkpointOnPosition = pad;
            play.game->gates().checkpointPlacement = map::Placement{
                .position = collision::positionOf(stoodPosition),
                .way = play.game->world()
                           .get<component::AnimationState>(play.game->player())
                           .direction};
            sayCaption("checkpoint", "the respawn is set here");
        }

    }

    void Editor::onSteppedDoors(const voxel::VoxelPosition standsInPosition)
    {
        const auto adjacentDoorCell = rules::adjacentDoor(
            document.map.doorPositions,
        standsInPosition);

        if (!adjacentDoorCell.has_value())
        {
            play.game->gates().announcedDoorPosition.reset();

            return;
        }

        if (play.game->gates().announcedDoorPosition == adjacentDoorCell)
        {
            return;
        }

        play.game->gates().announcedDoorPosition = adjacentDoorCell;

        if (play.game->gates().keysHeld == 0)
        {
            sayCaption(
                "the door", "locked - a key would open it");

            return;
        }

        pushUndo();

        const auto openedCells = rules::doorwayCells(document.map.doorPositions,
            *adjacentDoorCell);

        for (const auto doorCell : openedCells)
        {
            document.map.voxels = voxel::withRampsRebuilt(
                voxel::withoutBlockAt(document.map.voxels, doorCell), doorCell);
        }

        std::erase_if(
            document.map.doorPositions,
            [&openedCells](const voxel::VoxelPosition doorCell)
            {
                return std::find(
                           openedCells.begin(),
                           openedCells.end(),
                           doorCell)
                       != openedCells.end();
            });
        rebuildWorld();
        play.game->gates().keysHeld -= 1;
        sayCaption("the door", "a key unlocks it");
    }

    bool Editor::tryUnlockExit()
    {
        if (!document.map.exitLocked)
        {
            return true;
        }

        if (play.game->gates().keysHeld > 0)
        {
            play.game->gates().keysHeld -= 1;
            sayCaption("the exit", "unlocked");

            return true;
        }

        if (document.map.exitCubePosition.has_value()
            && play.game->gates().lockedExitAnnouncedPosition
                   != antwika::voxel::cubeCornerOf(
                       *document.map.exitCubePosition))
        {
            play.game->gates().lockedExitAnnouncedPosition =
                antwika::voxel::cubeCornerOf(*document.map.exitCubePosition);
            sayCaption(
                "the exit", "locked - a key would open it");
        }

        return false;
    }

}
