#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/collision/Collision.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Item.hpp>
#include <antwika/component/Pad.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/Entity.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/gameplay/Characters.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/editor/StoodCell.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {

        constexpr std::uint32_t kPickAgainTicks = 30;

        constexpr float kVoxelMiddle = 0.5F;

        constexpr std::array<EntityKind, component::kPadKindCount>
            kEntityKindsOfPad{
                EntityKind::Start,
                EntityKind::Exit,
                EntityKind::Checkpoint};

        [[nodiscard]] gfx::Vec3 getCellMiddle(
            const voxel::VoxelPosition cellPosition)
        {
            return gfx::Vec3{
                static_cast<float>(cellPosition.x) + kVoxelMiddle,
                static_cast<float>(cellPosition.y) + kVoxelMiddle,
                static_cast<float>(cellPosition.z) + kVoxelMiddle};
        }

    }

    void Editor::rebuildEntityRows()
    {
        auto &world = play.world;

        static_cast<void>(gameplay::relayPads(world, document.map));

        entityList.rows.clear();

        for (const auto entity : world.getLiveEntities())
        {
            EntityRow row;

            row.entity = entity;

            if (world.has<component::Pad>(entity))
            {
                const auto pad = world.get<component::Pad>(entity);

                row.kind = kEntityKindsOfPad.at(
                    enums::index(
                        static_cast<component::PadKind>(pad.kind)));
                row.cellPosition = pad.position;
                row.name =
                    std::string(kEntityNames.at(enums::index(*row.kind)));
                row.aimPosition = voxelmap::getCubeMiddle(pad.position);
            }
            else if (world.has<component::CharacterIndex>(entity))
            {
                const auto which = static_cast<std::size_t>(
                    world.get<component::CharacterIndex>(entity).index);

                row.kind = EntityKind::Character;
                row.characterIndex = which;
                row.name = "unnamed";

                if (which < document.map.characters.size())
                {
                    const auto &character =
                        document.map.characters.at(which);

                    row.player = character.player;
                    row.cellPosition = getStoodCell(
                        character.idlePlacement.position);

                    if (!character.name.empty())
                    {
                        row.name = character.name;
                    }
                }

                if (world.has<component::Position>(entity))
                {
                    row.aimPosition = collision::positionOf(
                        world.get<component::Position>(entity));
                }
            }
            else if (world.has<component::Item>(entity))
            {
                const auto &item = world.get<component::Item>(entity);

                row.kind =
                    static_cast<component::ItemKind>(item.kind)
                            == component::ItemKind::Water
                        ? EntityKind::Water
                        : EntityKind::Food;
                row.cellPosition = item.position;
                row.name =
                    std::string(kEntityNames.at(enums::index(*row.kind)));
                row.aimPosition = getCellMiddle(item.position);
            }
            else
            {
                row.name = "Entity " + std::to_string(getRawValue(entity));
            }

            entityList.rows.push_back(std::move(row));
        }
    }

    void Editor::layoutEntityListPanel(ui::Context &context)
    {
        rebuildEntityRows();

        {
            const auto entities = context.column(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::getFixedSize(
                        panelWidthOf(
                            &PanelSizes::entityWidth,
                            getInspectColumnWidth(
                                viewportRenderer.getWindowSize(),
                                camera::kCanvasSize))),
                    .heightSizing = antwika::ui::kGrowSizing,
                    .backgroundColor = kPanelColor,
                    .padding = kPanelPadding,
                    .widgetId = antwika::editor::kEntityListPanelWidget,
                    .clips = true});

            panelTitle(context, "Entities");

            const auto listing = context.scrollColumn(
                antwika::ui::ScrollSpec{
                    .widgetId = antwika::editor::kEntityListScrollWidget,
                    .heightSizing = antwika::ui::kGrowSizing,
                    .offset = entityList.scrollLine,
                    .dragging = entityList.trackHeld});

            const auto rowEnd =
                std::min(entityList.rows.size(), kMaxEntityRows);

            for (std::size_t place = 0; place < rowEnd; ++place)
            {
                const auto &row = entityList.rows.at(place);
                const auto isPicked =
                    row.kind.has_value() && entityPick.kind == row.kind
                    && (row.kind == EntityKind::Character
                            ? entityPick.characterIndex == row.characterIndex
                            : entityPick.position == row.cellPosition);

                context.button(
                    row.player ? row.name + " (player)" : row.name,
                    antwika::ui::ButtonSpec{
                        .widgetId = getEntityRowWidget(place),
                        .widthSizing = antwika::ui::kGrowSizing,
                        .fillColor = isPicked ? kSelectionAccentColor
                                              : kGridLineColor,
                        .labelAlignment =
                            antwika::ui::Alignment::Start});
            }
        }

        context.edge(
            antwika::ui::EdgeSpec{
                .widgetId = antwika::editor::kEntityListEdgeWidget,
                .panelWidget = antwika::editor::kEntityListPanelWidget,
                .minimum = kMinPanelWidth,
                .maximum = viewportRenderer.getWindowSize().width / 3,
                .dragging = pointer.heldEdgeWidget
                            == antwika::editor::kEntityListEdgeWidget});
    }

    bool Editor::pressEntityRow(const std::size_t place)
    {
        if (place >= entityList.rows.size())
        {
            return false;
        }

        const auto row = entityList.rows.at(place);
        const auto pickedAgain =
            pointer.lastPickedWidget == getEntityRowWidget(place)
            && tick < pointer.lastPickedAt + kPickAgainTicks;

        pointer.lastPickedWidget = getEntityRowWidget(place);
        pointer.lastPickedAt = tick;

        dropEntityPick();

        if (row.kind.has_value())
        {
            entityPick.kind = row.kind;
            entityPick.position = row.cellPosition;
            entityPick.characterIndex = row.characterIndex;
            preferences.tool = Tool::Select;
        }

        if (pickedAgain && row.aimPosition.has_value())
        {
            cameraRig.view.transform = camera::getAimedAt(
                cameraRig.view.transform, *row.aimPosition);
        }

        return true;
    }

    void Editor::carryEntityListScroll(const ui::Interactions &interactions)
    {
        if (interactions.scrollChange.has_value()
            && interactions.scrollChange->areaWidget
                   == kEntityListScrollWidget)
        {
            entityList.scrollLine = interactions.scrollChange->line;
        }

        if (interactions.activatedWidget != widget::kNoWidget)
        {
            entityList.trackHeld =
                interactions.areaPress.has_value()
                && interactions.areaPress->areaWidget
                       == kEntityListScrollWidget;
        }
    }

    bool Editor::isEntityListHovered() const
    {
        const auto hoveredWidget = pointer.hoveredWidget;

        if (hoveredWidget == widget::kNoWidget)
        {
            return false;
        }

        if (hoveredWidget == kEntityListPanelWidget
            || hoveredWidget == kEntityListScrollWidget)
        {
            return true;
        }

        return hoveredWidget >= kFirstEntityRowWidget
            && hoveredWidget
                   < getWidgetAfter(kFirstEntityRowWidget, kMaxEntityRows);
    }

}
