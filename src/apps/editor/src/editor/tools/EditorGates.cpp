#include <antwika/gameplay/GateState.hpp>
#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Vitals.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/rules/Gates.hpp>
#include <antwika/voxel/VoxelCell.hpp>
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
        const voxel::VoxelCell cell, const input::MouseButton button)
    {
        auto &gateCells = tool == map::Tool::Key    ? map.keyCells
                        : tool == map::Tool::Door  ? map.doorCells
                        : tool == map::Tool::Food  ? map.foodCells
                        : tool == map::Tool::Water ? map.waterCells
                        : map.checkpointCells;
        const auto foundCube = rules::gateCubeContaining(gateCells, cell);

        if (button == input::MouseButton::Right)
        {
            if (!foundCube.has_value())
            {
                return;
            }

            pushUndo();
            std::erase_if(
                gateCells,
                [this, corner = *foundCube](const voxel::VoxelCell one)
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
        gateCells.push_back(cell);

        if (!rules::cubeOccupied(map.voxels, antwika::voxel::cubeCornerOf(cell))
            || tool == map::Tool::Door)
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withBlockAt(map.voxels, cell), cell);
            rebuildWorld();
        }
    }

    void Editor::onSteppedWorld(const gfx::Vec3 walkerPosition)
    {
        const voxel::VoxelCell standsInCell{
            .x = static_cast<std::int32_t>(
                std::floor(walkerPosition.x / voxel::kVoxelSide)),
            .y = static_cast<std::int32_t>(
                std::floor(walkerPosition.y / voxel::kVoxelSide)),
            .z = static_cast<std::int32_t>(
                std::floor(walkerPosition.z / voxel::kVoxelSide))};
        const voxel::VoxelCell standsOnCell{
            .x = standsInCell.x,
            .y = standsInCell.y - 1,
            .z = standsInCell.z};

        onSteppedPlates(standsOnCell);
        onSteppedGates(standsInCell, standsOnCell);

        if (map.exitCubeCell.has_value()
            && (antwika::voxel::cubeCornerOf(standsInCell)
                    == antwika::voxel::cubeCornerOf(*map.exitCubeCell)
                || antwika::voxel::cubeCornerOf(standsOnCell)
                       == antwika::voxel::cubeCornerOf(*map.exitCubeCell)))
        {
            takeExit();
        }
    }

    void Editor::onSteppedGates(
        const voxel::VoxelCell standsInCell,
        const voxel::VoxelCell standsOnCell)
    {
        if (game->gates().lockedExitAnnouncedCell.has_value()
            && (!map.exitCubeCell.has_value()
                || (antwika::voxel::cubeCornerOf(standsInCell)
                        != antwika::voxel::cubeCornerOf(*map.exitCubeCell)
                    && antwika::voxel::cubeCornerOf(standsOnCell)
                           != antwika::voxel::cubeCornerOf(
                               *map.exitCubeCell))))
        {
            game->gates().lockedExitAnnouncedCell.reset();
        }

        onSteppedKeys(standsInCell, standsOnCell);
        onSteppedCheckpoints(standsOnCell);
        onSteppedDoors(standsInCell);
    }

    void Editor::onSteppedKeys(
        const voxel::VoxelCell standsInCell,
        const voxel::VoxelCell standsOnCell)
    {
        for (const auto gateCell : {standsInCell, standsOnCell})
        {
            const auto foundCube = rules::gateCubeContaining(map.keyCells,
            gateCell);

            if (!foundCube.has_value()
                || game->gates().collectedKeyCells.contains(*foundCube))
            {
                continue;
            }

            game->gates().collectedKeyCells.insert(*foundCube);
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
        const component::Vitals heldVitals{
            .health = world.get<component::Health>(game->player()),
            .inventory = world.get<component::Inventory>(game->player())};
        const auto afterVitals = rules::consumed(heldVitals, kind);

        if (afterVitals.inventory.slots == heldVitals.inventory.slots)
        {
            sayCaption(
                kind == component::ItemKind::Food ? "food" : "water",
                "there is none left to take");

            return;
        }

        world.set<component::Health>(game->player(), afterVitals.health);
        world.set<component::Inventory>(game->player(), afterVitals.inventory);
        world.commit();
        sayCaption(
            kind == component::ItemKind::Food ? "food" : "water",
            kind == component::ItemKind::Food ? "eaten" : "drunk");
    }

    void Editor::onSteppedCheckpoints(const voxel::VoxelCell standsOnCell)
    {
        const auto pad =
            rules::gateCubeContaining(map.checkpointCells, standsOnCell);

        if (pad.has_value() && game->gates().checkpointOnCell != pad)
        {
            const auto stoodPosition =
                game->world().get<component::Position>(game->player());

            game->gates().checkpointOnCell = pad;
            game->gates().checkpointPlacement = map::Placement{
                .position = collision::positionOf(stoodPosition),
                .way = game->world()
                           .get<component::AnimationState>(game->player())
                           .direction};
            sayCaption("checkpoint", "the respawn is set here");
        }

    }

    void Editor::onSteppedDoors(const voxel::VoxelCell standsInCell)
    {
        const auto adjacentDoorCell = rules::adjacentDoor(map.doorCells,
        standsInCell);

        if (!adjacentDoorCell.has_value())
        {
            game->gates().announcedDoorCell.reset();

            return;
        }

        if (game->gates().announcedDoorCell == adjacentDoorCell)
        {
            return;
        }

        game->gates().announcedDoorCell = adjacentDoorCell;

        if (game->gates().keysHeld == 0)
        {
            sayCaption(
                "the door", "locked - a key would open it");

            return;
        }

        pushUndo();

        const auto openedCells = rules::doorwayCells(map.doorCells,
            *adjacentDoorCell);

        for (const auto doorCell : openedCells)
        {
            map.voxels = voxel::withRampsRebuilt(
                voxel::withoutBlockAt(map.voxels, doorCell), doorCell);
        }

        std::erase_if(
            map.doorCells,
            [&openedCells](const voxel::VoxelCell doorCell)
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

        if (map.exitCubeCell.has_value()
            && game->gates().lockedExitAnnouncedCell
                   != antwika::voxel::cubeCornerOf(*map.exitCubeCell))
        {
            game->gates().lockedExitAnnouncedCell =
                antwika::voxel::cubeCornerOf(*map.exitCubeCell);
            sayCaption(
                "the exit", "locked - a key would open it");
        }

        return false;
    }

}
