#include "antwika/atlas_editor/SceneSnapshot.hpp"

#include <optional>

namespace antwika::atlas_editor
{

    SceneSnapshot snapshotOf(const EditorState &state)
    {
        return SceneSnapshot{
            .image = state.image().size(),
            .view = state.view(),
            .tiles = state.tiles(),
            .gridVisible = state.gridVisible(),
            // Turned off and having none come to the same picture.
            // So they are folded into one answer here.
            .guides = state.guidesVisible() ? state.guides()
                                            : std::nullopt,
            .hovered = state.hovered()};
    }

} // namespace antwika::atlas_editor
