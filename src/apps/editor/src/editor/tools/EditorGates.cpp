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
        game->gates() = gameplay::GateState{};
    }

    void Editor::clearAssignModes()
    {
        assignMode = AssignMode{};
    }

    void Editor::pressGate(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        auto &gateCells = tool == map::Tool::Key    ? map.keyPositions
                        : tool == map::Tool::Door  ? map.doorPositions
                        : tool == map::Tool::Food  ? map.foodPositions
                        : tool == map::Tool::Water ? map.waterPositions
                        : map.checkpointPositions;
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

                    if (gone && tool == map::Tool::Door)
                    {
                        map.voxels = voxel::withRampsRebuilt(
                            voxel::withoutBlockAt(map.voxels, one),
                            one);
                    }

                    return gone;
                });

            if (tool == map::Tool::Door)
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

        if (!rules::cubeOccupied(map.voxels,
            antwika::voxel::cubeCornerOf(position))
            || tool == map::Tool::Door)
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withBlockAt(map.voxels, position), position);
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

        if (map.exitCubePosition.has_value()
            && (antwika::voxel::cubeCornerOf(standsInPosition)
                    == antwika::voxel::cubeCornerOf(*map.exitCubePosition)
                || antwika::voxel::cubeCornerOf(standsOnPosition)
                       == antwika::voxel::cubeCornerOf(*map.exitCubePosition)))
        {
            takeExit();
        }
    }

    void Editor::onSteppedGates(
        const voxel::VoxelPosition standsInPosition,
        const voxel::VoxelPosition standsOnPosition)
    {
        if (game->gates().lockedExitAnnouncedPosition.has_value()
            && (!map.exitCubePosition.has_value()
                || (antwika::voxel::cubeCornerOf(standsInPosition)
                        != antwika::voxel::cubeCornerOf(*map.exitCubePosition)
                    && antwika::voxel::cubeCornerOf(standsOnPosition)
                           != antwika::voxel::cubeCornerOf(
                               *map.exitCubePosition))))
        {
            game->gates().lockedExitAnnouncedPosition.reset();
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
            const auto foundCube = rules::gateCubeContaining(map.keyPositions,
            gateCell);

            if (!foundCube.has_value()
                || game->gates().collectedKeyPositions.contains(*foundCube))
            {
                continue;
            }

            game->gates().collectedKeyPositions.insert(*foundCube);
            game->gates().keysHeld += 1;
            sayCaption(
                "a key",
                "taken (" + std::to_string(game->gates().keysHeld)
                    + " held)");
        }

    }

    void Editor::consumeItem(const component::ItemKind kind)
    {
        auto &world = game->world();
        const ecs::OpenPhase phase(world);

        world.add<component::ConsumeIntent>(
            game->player(),
            component::ConsumeIntent{
                .kind = static_cast<std::uint8_t>(kind)});
    }

    void Editor::sayConsumeReport()
    {
        auto &world = game->world();

        if (!world.has<component::ConsumeReport>(game->player()))
        {
            return;
        }

        const auto report =
            world.get<component::ConsumeReport>(game->player());
        const auto kind = static_cast<component::ItemKind>(report.kind);

        sayCaption(
            kind == component::ItemKind::Food ? "food" : "water",
            report.anyLeft
                ? (kind == component::ItemKind::Food ? "eaten" : "drunk")
                : "there is none left to take");

        const ecs::OpenPhase phase(world);

        world.remove<component::ConsumeReport>(game->player());
    }

    void Editor::onSteppedCheckpoints(
        const voxel::VoxelPosition standsOnPosition)
    {
        const auto pad =
            rules::gateCubeContaining(map.checkpointPositions,
                standsOnPosition);

        if (pad.has_value() && game->gates().checkpointOnPosition != pad)
        {
            const auto stoodPosition =
                game->world().get<component::Position>(game->player());

            game->gates().checkpointOnPosition = pad;
            game->gates().checkpointPlacement = map::Placement{
                .position = collision::positionOf(stoodPosition),
                .way = game->world()
                           .get<component::AnimationState>(game->player())
                           .direction};
            sayCaption("checkpoint", "the respawn is set here");
        }

    }

    void Editor::onSteppedDoors(const voxel::VoxelPosition standsInPosition)
    {
        const auto adjacentDoorCell = rules::adjacentDoor(map.doorPositions,
        standsInPosition);

        if (!adjacentDoorCell.has_value())
        {
            game->gates().announcedDoorPosition.reset();

            return;
        }

        if (game->gates().announcedDoorPosition == adjacentDoorCell)
        {
            return;
        }

        game->gates().announcedDoorPosition = adjacentDoorCell;

        if (game->gates().keysHeld == 0)
        {
            sayCaption(
                "the door", "locked - a key would open it");

            return;
        }

        pushUndo();

        const auto openedCells = rules::doorwayCells(map.doorPositions,
            *adjacentDoorCell);

        for (const auto doorCell : openedCells)
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withoutBlockAt(map.voxels, doorCell), doorCell);
        }

        std::erase_if(
            map.doorPositions,
            [&openedCells](const voxel::VoxelPosition doorCell)
            {
                return std::find(
                           openedCells.begin(),
                           openedCells.end(),
                           doorCell)
                       != openedCells.end();
            });
        rebuildWorld();
        game->gates().keysHeld -= 1;
        sayCaption("the door", "a key unlocks it");
    }

    bool Editor::tryUnlockExit()
    {
        if (!map.exitLocked)
        {
            return true;
        }

        if (game->gates().keysHeld > 0)
        {
            game->gates().keysHeld -= 1;
            sayCaption("the exit", "unlocked");

            return true;
        }

        if (map.exitCubePosition.has_value()
            && game->gates().lockedExitAnnouncedPosition
                   != antwika::voxel::cubeCornerOf(*map.exitCubePosition))
        {
            game->gates().lockedExitAnnouncedPosition =
                antwika::voxel::cubeCornerOf(*map.exitCubePosition);
            sayCaption(
                "the exit", "locked - a key would open it");
        }

        return false;
    }

}
