#include "antwika/game/UiOverlay.hpp"

#include <cstdint>
#include <utility>
#include <variant>

#include <antwika/gfx/Rect.hpp>

namespace antwika::game
{

    namespace
    {
        // Half-open on the far edges, which is ui::contains()' own rule.
        // Two boxes sharing an edge then claim one pixel between them.
        [[nodiscard]] bool inside(
            antwika::gfx::Rect box, antwika::gfx::Point at) noexcept
        {
            return at.x >= box.origin.x
                   && at.x < box.origin.x
                                 + static_cast<std::int32_t>(
                                     box.size.width)
                   && at.y >= box.origin.y
                   && at.y < box.origin.y
                                 + static_cast<std::int32_t>(
                                     box.size.height);
        }
    } // namespace

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

    bool UiOverlay::coversPoint(antwika::gfx::Point at) const noexcept
    {
        for (const auto &command : picture)
        {
            const auto *fill = std::get_if<antwika::ui::FillRect>(&command);

            if (fill != nullptr && inside(fill->rect, at))
            {
                return true;
            }
        }

        return false;
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
