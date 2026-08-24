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
        play.game->getGates() = gameplay::GateState{};
    }

    void Editor::clearAssignModes()
    {
        assignMode = AssignMode{};
    }

    void Editor::pressGate(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        const auto chosenTool = preferences.tool;

        auto &drawnMap = document.map;

        auto &gateCells = drawnMap.markers.positionsOf(
            map::markerFor(chosenTool).value_or(map::Marker::Checkpoint));
        const auto foundCube = rules::getGateCubeContaining(gateCells, position);

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

                    if (gone && preferences.tool == map::Tool::Door)
                    {
                        document.map.voxels = voxel::getWithRampsRebuilt(
                            voxel::withoutBlockAt(document.map.voxels, one),
                            one);
                    }

                    return gone;
                });

            if (preferences.tool == map::Tool::Door)
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

        if (!rules::isCubeOccupied(document.map.voxels,
            antwika::voxel::cubeCornerOf(position))
            || preferences.tool == map::Tool::Door)
        {
            document.map.voxels = voxel::getWithRampsRebuilt(
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
        if (play.game->getGates().lockedExitAnnouncedPosition.has_value()
            && (!document.map.exitCubePosition.has_value()
                || (antwika::voxel::cubeCornerOf(standsInPosition)
                        != antwika::voxel::cubeCornerOf(
                            *document.map.exitCubePosition)
                    && antwika::voxel::cubeCornerOf(standsOnPosition)
                           != antwika::voxel::cubeCornerOf(
                               *document.map.exitCubePosition))))
        {
            play.game->getGates().lockedExitAnnouncedPosition.reset();
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
            const auto foundCube = rules::getGateCubeContaining(
                document.map.markers.positionsOf(map::Marker::Key),
            gateCell);

            if (!foundCube.has_value()
                || play.game->getGates().collectedKeyPositions.contains(
                    *foundCube))
            {
                continue;
            }

            play.game->getGates().collectedKeyPositions.insert(*foundCube);
            play.game->getGates().keysHeld += 1;
            sayCaption(
                "a key",
                "taken (" + std::to_string(play.game->getGates().keysHeld)
                    + " held)");
        }

    }

    void Editor::consumeItem(const component::ItemKind kind)
    {
        auto &gameWorld = play.game->getWorld();
        const ecs::OpenPhase phase(gameWorld);

        gameWorld.add<component::ConsumeIntent>(
            play.game->getPlayer(),
            component::ConsumeIntent{
                .kind = static_cast<std::uint8_t>(kind)});
    }

    void Editor::sayConsumeReport()
    {
        auto &gameWorld = play.game->getWorld();

        if (!gameWorld.has<component::ConsumeReport>(play.game->getPlayer()))
        {
            return;
        }

        const auto report =
            gameWorld.get<component::ConsumeReport>(play.game->getPlayer());
        const auto kind = static_cast<component::ItemKind>(report.kind);

        sayCaption(
            kind == component::ItemKind::Food ? "food" : "water",
            report.anyLeft
                ? (kind == component::ItemKind::Food ? "eaten" : "drunk")
                : "there is none left to take");

        const ecs::OpenPhase phase(gameWorld);

        gameWorld.remove<component::ConsumeReport>(play.game->getPlayer());
    }

    void Editor::onSteppedCheckpoints(
        const voxel::VoxelPosition standsOnPosition)
    {
        const auto pad =
            rules::getGateCubeContaining(document.map.markers.positionsOf(map::Marker::Checkpoint),
                standsOnPosition);

        if (pad.has_value() && play.game->getGates().checkpointOnPosition != pad)
        {
            const auto stoodPosition =
                play.game->getWorld().get<component::Position>(play.game->getPlayer(
                        ));

            play.game->getGates().checkpointOnPosition = pad;
            play.game->getGates().checkpointPlacement = map::Placement{
                .position = collision::positionOf(stoodPosition),
                .way = play.game->getWorld()
                           .get<component::AnimationState>(play.game->getPlayer())
                           .direction};
            sayCaption("checkpoint", "the respawn is set here");
        }

    }

    void Editor::onSteppedDoors(const voxel::VoxelPosition standsInPosition)
    {
        const auto adjacentDoorCell = rules::getAdjacentDoor(
            document.map.markers.positionsOf(map::Marker::Door),
        standsInPosition);

        if (!adjacentDoorCell.has_value())
        {
            play.game->getGates().announcedDoorPosition.reset();

            return;
        }

        if (play.game->getGates().announcedDoorPosition == adjacentDoorCell)
        {
            return;
        }

        play.game->getGates().announcedDoorPosition = adjacentDoorCell;

        if (play.game->getGates().keysHeld == 0)
        {
            sayCaption(
                "the door", "locked - a key would open it");

            return;
        }

        pushUndo();

        const auto openedCells = rules::getDoorwayCells(
            document.map.markers.positionsOf(map::Marker::Door),
            *adjacentDoorCell);

        for (const auto doorCell : openedCells)
        {
            document.map.voxels = voxel::getWithRampsRebuilt(
                voxel::withoutBlockAt(document.map.voxels, doorCell), doorCell);
        }

        std::erase_if(
            document.map.markers.positionsOf(map::Marker::Door),
            [&openedCells](const voxel::VoxelPosition doorCell)
            {
                return std::find(
                           openedCells.begin(),
                           openedCells.end(),
                           doorCell)
                       != openedCells.end();
            });
        rebuildWorld();
        play.game->getGates().keysHeld -= 1;
        sayCaption("the door", "a key unlocks it");
    }

    bool Editor::tryUnlockExit()
    {
        if (!document.map.exitLocked)
        {
            return true;
        }

        if (play.game->getGates().keysHeld > 0)
        {
            play.game->getGates().keysHeld -= 1;
            sayCaption("the exit", "unlocked");

            return true;
        }

        if (document.map.exitCubePosition.has_value()
            && play.game->getGates().lockedExitAnnouncedPosition
                   != antwika::voxel::cubeCornerOf(
                       *document.map.exitCubePosition))
        {
            play.game->getGates().lockedExitAnnouncedPosition =
                antwika::voxel::cubeCornerOf(*document.map.exitCubePosition);
            sayCaption(
                "the exit", "locked - a key would open it");
        }

        return false;
    }

}
