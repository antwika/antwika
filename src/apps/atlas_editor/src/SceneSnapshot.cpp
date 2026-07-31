#include "antwika/atlas_editor/SceneSnapshot.hpp"

namespace antwika::atlas_editor
{

    SceneSnapshot snapshotOf(const EditorState &state)
    {
        return SceneSnapshot{
            .image = state.image().size(),
            .view = state.view(),
            .tiles = state.tiles(),
            .gridVisible = state.gridVisible(),
            .hovered = state.hovered()};
    }

} // namespace antwika::atlas_editor
