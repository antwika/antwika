#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    struct HintKey final
    {
        EditorView view = EditorView::Map;
        ui::WidgetId widget = ui::kNoWidget;
        std::optional<gfx::Point> pointer{};
        bool modal = false;
        std::size_t edits = 0;
        std::int32_t level = 0;
        std::size_t tilesState = 0;
        std::size_t selectionState = 0;
        std::size_t pickerState = 0;

        [[nodiscard]] bool operator==(const HintKey &other) const =
            default;
    };

    /**
     * @brief The cache key for the hovered target's hint.
     *
     * Ensures: the key changes with the view, the hovered widget,
     *          the pointer position, modality, the undo depth, the
     *          active level, the tiles workspace state, the
     *          selection state, and the picker walk, so a cached
     *          hint never outlives an edit under a still pointer.
     */
    [[nodiscard]] HintKey hintKeyFor(
        const EditorStore &store, ui::WidgetId hovered);

    /**
     * @brief One line describing what the pointer points at.
     *
     * @param hovered The hovered widget from the ui interactions.
     * @return The hint, or an empty string when there is nothing
     *         meaningful to say.
     *
     * Ensures: while a modal dialog is open only its own widgets
     *          are described, never the map underneath.
     */
    [[nodiscard]] std::string hintFor(
        const EditorStore &store, ui::WidgetId hovered);

}
