#include "antwika/atlas_editor/UiOverlay.hpp"

#include <utility>

namespace antwika::atlas_editor
{

    void UiOverlay::set(
        DrawList commands, const bool over, const PaneRects panes)
    {
        picture = std::move(commands);
        covered = over;
        paneRects = panes;
    }

    const DrawList &UiOverlay::commands() const noexcept
    {
        return picture;
    }

    bool UiOverlay::pointerOverUi() const noexcept
    {
        return covered;
    }

    const PaneRects &UiOverlay::panes() const noexcept
    {
        return paneRects;
    }

}
