#include <antwika/editor/ui/CharacterSheetView.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::drawCharacterView(
        const ui::Frame &uiFrame,
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        characterView.draw(viewportRenderer);

        finishView(uiFrame, startedAt);
    }

}
