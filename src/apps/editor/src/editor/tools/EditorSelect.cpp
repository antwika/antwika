#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/rules/MarkerCubes.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/WorldCamera.hpp"
#include "antwika/editor/editor/StoodCell.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {

        [[nodiscard]] std::optional<std::int32_t> getWholeNumberFrom(
            const std::string &text)
        {
            std::int32_t value = 0;
            const auto [rest, mishap] = std::from_chars(
                text.data(), text.data() + text.size(), value);

            if (mishap != std::errc{}
                || rest != text.data() + text.size())
            {
                return std::nullopt;
            }

            return value;
        }

    }

    void Editor::pressSelect(
        const voxel::VoxelPosition position, const input::MouseButton button)
    {
        if (button == input::MouseButton::Right)
        {
            dropEntityPick();

            return;
        }

        if (button != input::MouseButton::Left)
        {
            return;
        }

        const auto foundEntity = findEntityAt(position);

        dropEntityPick();

        if (!foundEntity.has_value())
        {
            return;
        }

        entityPick = *foundEntity;
        entityPick.dragging = true;
    }

    std::optional<EntityPick> Editor::findEntityAt(
        const voxel::VoxelPosition position) const
    {
        const auto corner = antwika::voxel::cubeCornerOf(position);

        for (std::size_t index = 0;
             index < document.map.characters.size();
             ++index)
        {
            const auto stoodCell = getStoodCell(
                document.map.characters.at(index).idlePlacement.position);
            const auto stoodCorner = antwika::voxel::cubeCornerOf(stoodCell);

            if (stoodCorner.x == corner.x && stoodCorner.z == corner.z)
            {
                EntityPick picked;

                picked.kind = EntityKind::Character;
                picked.position = stoodCell;
                picked.characterIndex = index;

                return picked;
            }
        }

        for (const auto &lamp : document.map.lamps)
        {
            if (antwika::voxel::cubeCornerOf(lamp.position) == corner)
            {
                EntityPick picked;

                picked.kind = EntityKind::Lamp;
                picked.position = lamp.position;

                return picked;
            }
        }

        if (document.map.spawnCubePosition.has_value()
            && antwika::voxel::cubeCornerOf(*document.map.spawnCubePosition)
                   == corner)
        {
            EntityPick picked;

            picked.kind = EntityKind::Start;
            picked.position = *document.map.spawnCubePosition;

            return picked;
        }

        if (document.map.exitCubePosition.has_value()
            && antwika::voxel::cubeCornerOf(*document.map.exitCubePosition)
                   == corner)
        {
            EntityPick picked;

            picked.kind = EntityKind::Exit;
            picked.position = *document.map.exitCubePosition;

            return picked;
        }

        for (const auto kind :
             {EntityKind::Checkpoint, EntityKind::Food, EntityKind::Water})
        {
            const auto &cells = document.map.markers.positionsOf(
                *getMarkerOfEntity(kind));
            const auto foundCell = std::ranges::find_if(
                cells,
                [corner](const voxel::VoxelPosition one)
                { return antwika::voxel::cubeCornerOf(one) == corner; });

            if (foundCell != cells.end())
            {
                EntityPick picked;

                picked.kind = kind;
                picked.position = *foundCell;

                return picked;
            }
        }

        return std::nullopt;
    }

    void Editor::keepEntityStep()
    {
        if (entityPick.dragging && entityPick.dragUndoKept)
        {
            return;
        }

        pushUndo();

        if (entityPick.dragging)
        {
            entityPick.dragUndoKept = true;
        }
    }

    bool Editor::moveEntityTo(
        const voxel::VoxelPosition nextPosition, const bool snapsToGround)
    {
        if (!entityPick.kind.has_value())
        {
            return false;
        }

        switch (*entityPick.kind)
        {
        case EntityKind::Start:
        case EntityKind::Exit:
        {
            auto &landing = *entityPick.kind == EntityKind::Start
                          ? document.map.spawnCubePosition
                          : document.map.exitCubePosition;

            keepEntityStep();
            landing = nextPosition;

            break;
        }
        case EntityKind::Lamp:
        {
            const auto carriedLamp = std::ranges::find_if(
                document.map.lamps,
                [this](const light::Lamp &lamp)
                { return lamp.position == entityPick.position; });
            const auto blockedCell = std::ranges::any_of(
                document.map.lamps,
                [nextCorner = antwika::voxel::cubeCornerOf(nextPosition)](
                    const light::Lamp &lamp)
                {
                    return antwika::voxel::cubeCornerOf(lamp.position)
                           == nextCorner;
                });

            if (carriedLamp == document.map.lamps.end() || blockedCell)
            {
                return false;
            }

            keepEntityStep();
            document.map.lamps = light::withLampAt(
                light::withoutLampAt(
                    document.map.lamps, entityPick.position),
                nextPosition,
                carriedLamp->tintColor);
            lightPasses.forget();

            break;
        }
        case EntityKind::Checkpoint:
        case EntityKind::Food:
        case EntityKind::Water:
        {
            auto &cells = document.map.markers.positionsOf(
                *getMarkerOfEntity(*entityPick.kind));
            const auto foundCell =
                std::ranges::find(cells, entityPick.position);
            const auto blockedCube = std::ranges::any_of(
                cells,
                [former = entityPick.position,
                 nextCorner = antwika::voxel::cubeCornerOf(nextPosition)](
                    const voxel::VoxelPosition one)
                {
                    return one != former
                           && antwika::voxel::cubeCornerOf(one)
                                  == nextCorner;
                });

            if (foundCell == cells.end() || blockedCube)
            {
                return false;
            }

            keepEntityStep();
            cells.erase(foundCell);
            cells.push_back(nextPosition);

            break;
        }
        case EntityKind::Character:
        {
            if (entityPick.characterIndex >= document.map.characters.size())
            {
                return false;
            }

            auto standingHeight =
                static_cast<float>(nextPosition.y) * voxel::kVoxelSide;

            if (snapsToGround)
            {
                const auto feet =
                    (static_cast<float>(nextPosition.y) + 0.5F)
                    * voxel::kVoxelSide;
                const auto groundHeight =
                    collision::getGroundHeightAtColumn(
                        worldMeshes.getCells(),
                        nextPosition.x,
                        nextPosition.z,
                        feet);

                if (!groundHeight.has_value())
                {
                    return false;
                }

                standingHeight = *groundHeight;
            }

            keepEntityStep();
            document.map.characters.at(entityPick.characterIndex)
                .idlePlacement.position = antwika::gfx::Vec3{
                static_cast<float>(nextPosition.x) * voxel::kVoxelSide,
                standingHeight,
                static_cast<float>(nextPosition.z) * voxel::kVoxelSide};
            spawnCharacters();

            break;
        }
        }

        entityPick.position = nextPosition;

        return true;
    }

    void Editor::carryEntity()
    {
        if (!entityPick.dragging || !entityPick.kind.has_value()
            || !isWorldShown() || play.playing)
        {
            return;
        }

        const auto cell = voxelmap::getCellUnder(
            getWorldCamera(play, cameraRig),
            getWorldRotation(play),
            camera::kCanvasSize,
            pointer.pointerOnCanvas,
            antwika::voxel::getCubeTop(
                worldView.worldEdit().getEditLevel()));

        if (!cell.has_value() || *cell == entityPick.position)
        {
            return;
        }

        moveEntityTo(*cell, true);
    }

    void Editor::removeEntityPick()
    {
        if (!isEntitySectionShown())
        {
            return;
        }

        switch (*entityPick.kind)
        {
        case EntityKind::Start:
            pushUndo();
            document.map.spawnCubePosition.reset();

            break;
        case EntityKind::Exit:
            pushUndo();
            document.map.exitCubePosition.reset();

            break;
        case EntityKind::Lamp:
            pushUndo();
            document.map.lamps = light::withoutLampAt(
                document.map.lamps, entityPick.position);
            lightPasses.forget();

            break;
        case EntityKind::Checkpoint:
        case EntityKind::Food:
        case EntityKind::Water:
        {
            auto &cells = document.map.markers.positionsOf(
                *getMarkerOfEntity(*entityPick.kind));

            pushUndo();
            std::erase_if(
                cells,
                [corner = antwika::voxel::cubeCornerOf(
                     entityPick.position)](const voxel::VoxelPosition one)
                {
                    return antwika::voxel::cubeCornerOf(one) == corner;
                });

            break;
        }
        case EntityKind::Character:
            if (document.map.characters.at(entityPick.characterIndex)
                    .player)
            {
                showStatus("the player stays", false, 120);

                return;
            }

            pushUndo();
            document.map.characters.erase(
                std::next(
                    document.map.characters.begin(),
                    static_cast<std::ptrdiff_t>(
                        entityPick.characterIndex)));
            worldView.characterTool().dropChoice();
            spawnCharacters();
            loadCharacterSkins();

            break;
        }

        dropEntityPick();
    }

    void Editor::dropEntityPick()
    {
        entityPick = EntityPick{};

        if (focusedField == FocusedField::EntityAxis)
        {
            focusedField = FocusedField::Nothing;
        }
    }

    void Editor::commitEntityEdit()
    {
        const auto axis = entityPick.editingAxis;
        const auto value = getWholeNumberFrom(entityPick.pendingAxisText);

        entityPick.editingAxis.reset();
        entityPick.pendingAxisText.clear();

        if (!axis.has_value() || !value.has_value()
            || !entityPick.kind.has_value())
        {
            return;
        }

        const auto nextPosition =
            getWithCubeAxisSet(entityPick.position, *axis, *value);

        if (nextPosition == entityPick.position)
        {
            return;
        }

        moveEntityTo(nextPosition, false);
    }

    bool Editor::isEntitySectionShown() const
    {
        if (preferences.tool != Tool::Select || !isWorldShown()
            || !entityPick.kind.has_value())
        {
            return false;
        }

        switch (*entityPick.kind)
        {
        case EntityKind::Start:
            return document.map.spawnCubePosition == entityPick.position;
        case EntityKind::Exit:
            return document.map.exitCubePosition == entityPick.position;
        case EntityKind::Lamp:
            return std::ranges::any_of(
                document.map.lamps,
                [this](const light::Lamp &lamp)
                { return lamp.position == entityPick.position; });
        case EntityKind::Checkpoint:
        case EntityKind::Food:
        case EntityKind::Water:
        {
            const auto &cells = document.map.markers.positionsOf(
                *getMarkerOfEntity(*entityPick.kind));

            return std::ranges::find(cells, entityPick.position)
                   != cells.end();
        }
        case EntityKind::Character:
            return entityPick.characterIndex
                   < document.map.characters.size();
        }

        return false;
    }

    void Editor::layoutEntitySection(ui::Context &context)
    {
        const auto entityPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(
            context, kEntityNames.at(enums::index(*entityPick.kind)));

        if (*entityPick.kind == EntityKind::Character)
        {
            const auto &name =
                document.map.characters.at(entityPick.characterIndex).name;

            context.label(
                name.empty() ? "unnamed" : name, kGridLineColor);
        }

        constexpr std::array<std::string_view, kMarkerAxisCount>
            kAxisNames{"x", "y", "z"};

        for (std::size_t axis = 0; axis < kMarkerAxisCount; ++axis)
        {
            const auto editing = focusedField == FocusedField::EntityAxis
                                 && entityPick.editingAxis == axis;
            const auto axisRow = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            context.label(kAxisNames.at(axis), kTextColor);
            context.textField(
                antwika::ui::TextFieldSpec{
                    .widgetId = getEntityFieldWidget(axis),
                    .text = editing
                                ? entityPick.pendingAxisText
                                : std::to_string(
                                      getCubeAxisOf(
                                          entityPick.position, axis)),
                    .focused = editing});
        }

        context.button(
            "remove",
            antwika::ui::ButtonSpec{
                .widgetId = antwika::editor::kEntityRemoveWidget,
                .widthSizing = antwika::ui::kGrowSizing});
    }

}
