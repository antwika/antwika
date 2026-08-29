#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Editor.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    namespace
    {

        constexpr std::array<std::string_view, enums::kCount<map::Marker>>
            kMarkerNames{"Checkpoint", "Food", "Water"};

        constexpr std::array<std::string_view, kMarkerAxisCount>
            kAxisNames{"x", "y", "z"};

        [[nodiscard]] std::optional<std::int32_t> getWholeFrom(
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

    bool Editor::isMarkerSectionShown() const
    {
        const auto kind = getMarkerOf(preferences.tool);

        if (!isWorldShown() || !kind.has_value()
            || markerPick.marker != kind)
        {
            return false;
        }

        const auto &cells = document.map.markers.positionsOf(*kind);

        return std::ranges::find(cells, markerPick.position)
               != cells.end();
    }

    void Editor::layoutMarkerSection(ui::Context &context)
    {
        const auto markerPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(
            context, kMarkerNames.at(enums::index(*markerPick.marker)));

        for (std::size_t axis = 0; axis < kMarkerAxisCount; ++axis)
        {
            const auto editing = focusedField == FocusedField::MarkerAxis
                                 && markerPick.editingAxis == axis;
            const auto axisRow = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            context.label(kAxisNames.at(axis), kTextColor);
            context.textField(
                antwika::ui::TextFieldSpec{
                    .widgetId = getMarkerFieldWidget(axis),
                    .text = editing
                                ? markerPick.pendingAxisText
                                : std::to_string(
                                      getCubeAxisOf(
                                          markerPick.position, axis)),
                    .focused = editing});
        }

        context.button(
            "remove",
            antwika::ui::ButtonSpec{
                .widgetId = antwika::editor::kMarkerRemoveWidget,
                .widthSizing = antwika::ui::kGrowSizing});
    }

    void Editor::commitMarkerEdit()
    {
        const auto axis = markerPick.editingAxis;
        const auto value = getWholeFrom(markerPick.pendingAxisText);

        markerPick.editingAxis.reset();
        markerPick.pendingAxisText.clear();

        if (!axis.has_value() || !value.has_value()
            || !markerPick.marker.has_value())
        {
            return;
        }

        const auto nextPosition =
            getWithCubeAxisSet(markerPick.position, *axis, *value);

        if (nextPosition == markerPick.position)
        {
            return;
        }

        auto &cells = document.map.markers.positionsOf(*markerPick.marker);
        const auto foundCell =
            std::ranges::find(cells, markerPick.position);
        const auto blockedCube = std::ranges::any_of(
            cells,
            [former = markerPick.position,
             nextCorner = antwika::voxel::cubeCornerOf(nextPosition)](
                const voxel::VoxelPosition one)
            {
                return one != former
                       && antwika::voxel::cubeCornerOf(one) == nextCorner;
            });

        if (foundCell == cells.end() || blockedCube)
        {
            return;
        }

        pushUndo();
        cells.erase(foundCell);
        cells.push_back(nextPosition);
        markerPick.position = nextPosition;
    }

    void Editor::dropMarkerPick()
    {
        markerPick = MarkerPick{};

        if (focusedField == FocusedField::MarkerAxis)
        {
            focusedField = FocusedField::Nothing;
        }
    }

}
