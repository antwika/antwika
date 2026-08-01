#include "antwika/game/UiOverlay.hpp"

#include <utility>

namespace antwika::game
{

    UiOverlay::UiOverlay(Size canvas) : area(canvas)
    {
    }

    Size UiOverlay::canvas() const noexcept
    {
        return area;
    }

    void UiOverlay::set(DrawList commands, bool covered)
    {
        picture = std::move(commands);
        this->covered = covered;
    }

    const DrawList &UiOverlay::commands() const noexcept
    {
        return picture;
    }

    bool UiOverlay::pointerOverUi() const noexcept
    {
        return covered;
    }

    void UiOverlay::select(BuildTool tool) noexcept
    {
        selected = tool;
    }

    void UiOverlay::clearTool() noexcept
    {
        selected.reset();
    }

    std::optional<BuildTool> UiOverlay::tool() const noexcept
    {
        return selected;
    }

} // namespace antwika::game
