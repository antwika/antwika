#include "antwika/atlas_editor/SceneSnapshot.hpp"

#include <cstdint>
#include <optional>

namespace antwika::atlas_editor
{

    namespace
    {
        [[nodiscard]] std::optional<Rect> slotRectOf(
            const EditorState &state)
        {
            const auto focused = state.preview().focused;

            if (!focused.has_value())
            {
                return std::nullopt;
            }

            const auto tiles = state.tiles();
            const auto across = columnsIn(tiles, state.image().size());

            if (across == 0)
            {
                return std::nullopt;
            }

            return Rect{
                .origin = {
                    .x = static_cast<std::int32_t>(
                        (*focused % across) * tiles.width),
                    .y = static_cast<std::int32_t>(
                        (*focused / across) * tiles.height)},
                .size = {.width = tiles.width, .height = tiles.height}};
        }

        [[nodiscard]] std::optional<PreviewShot> shotOf(
            const EditorState &state, const std::optional<Rect> &pane)
        {
            if (!state.preview().open || !pane.has_value())
            {
                return std::nullopt;
            }

            return PreviewShot{
                .pane = *pane,
                .view = state.preview().view,
                .slot = slotRectOf(state)};
        }
    }

    SceneSnapshot snapshotOf(const EditorState &state)
    {
        return snapshotOf(state, std::nullopt);
    }

    SceneSnapshot snapshotOf(
        const EditorState &state, const std::optional<Rect> &pane)
    {
        return SceneSnapshot{
            .image = state.image().size(),
            .view = state.view(),
            .tiles = state.tiles(),
            .gridVisible = state.gridVisible(),
            .pixelGridVisible = state.pixelGridVisible(),
            .guides = state.guidesVisible() ? state.guides()
                                            : std::nullopt,
            .pivot = state.pivotVisible()
                         ? std::optional{state.meta().pivot}
                         : std::nullopt,
            .pointerBorder = state.pointerBorderVisible(),
            .selection = state.shownSelection(),
            .tool = state.tool(),
            .stroke = state.shownStroke(),
            .hovered = state.hovered(),
            .preview = shotOf(state, pane)};
    }

}
