#include <antwika/editor/ui/IconsView.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::drawIconsView(
        const ui::Frame &frame,
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        iconsView.draw(viewportRenderer);

        finishView(frame, startedAt);
    }

}
