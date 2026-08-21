#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::drawPlanView(
        const ui::Frame &frame,
        const std::chrono::time_point<std::chrono::system_clock>
            startedAt)
    {
        finishView(frame, startedAt);
    }

}
