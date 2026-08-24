#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::pressedOnSheets(
        const input::PointerButtonPressed &downPressed)
    {
        if (auto *view = viewNow();
            view != nullptr
            && view->consumePress(viewContextNow(), downPressed))
        {
            return;
        }
    }

}
