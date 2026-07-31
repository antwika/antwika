#include "antwika/atlas_editor/UiOverlay.hpp"

#include <utility>

namespace antwika::atlas_editor
{

    void UiOverlay::set(DrawList commands, const bool over)
    {
        picture = std::move(commands);
        covered = over;
    }

    const DrawList &UiOverlay::commands() const noexcept
    {
        return picture;
    }

    bool UiOverlay::pointerOverUi() const noexcept
    {
        return covered;
    }

} // namespace antwika::atlas_editor
