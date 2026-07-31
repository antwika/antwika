#include "antwika/ui_demo/DemoState.hpp"

#include <utility>

namespace antwika::ui_demo
{

    Showcase DemoState::showcase() const noexcept
    {
        return page;
    }

    std::size_t DemoState::selected() const noexcept
    {
        return static_cast<std::size_t>(page);
    }

    void DemoState::select(const std::size_t index) noexcept
    {
        if (index >= kShowcaseCount)
        {
            return;
        }

        page = static_cast<Showcase>(index);
    }

    bool DemoState::pickerOpen() const noexcept
    {
        return pickerShowing;
    }

    void DemoState::setPickerOpen(const bool showing) noexcept
    {
        pickerShowing = showing;
    }

    std::size_t DemoState::accent() const noexcept
    {
        return chosenAccent;
    }

    void DemoState::selectAccent(const std::size_t index) noexcept
    {
        // Anything outside the list selects nothing.
        // Which is what the placeholder is drawn for.
        chosenAccent =
            index < kAccentCount ? index : antwika::ui::kNoOption;
    }

    bool DemoState::accentOpen() const noexcept
    {
        return accentShowing;
    }

    void DemoState::setAccentOpen(const bool showing) noexcept
    {
        accentShowing = showing;
    }

    const std::string &DemoState::text() const noexcept
    {
        return typed;
    }

    std::size_t DemoState::caret() const noexcept
    {
        return cursor;
    }

    void DemoState::setText(std::string characters, const std::size_t at)
    {
        typed = std::move(characters);
        cursor = at;
    }

    WidgetId DemoState::focus() const noexcept
    {
        return focused;
    }

    void DemoState::setFocus(const WidgetId id) noexcept
    {
        focused = id;
    }

    std::uint32_t DemoState::clicks() const noexcept
    {
        return clickCount;
    }

    void DemoState::countClick() noexcept
    {
        ++clickCount;
    }

    void DemoState::resetClicks() noexcept
    {
        clickCount = 0;
    }

    const std::string &DemoState::message() const noexcept
    {
        return note;
    }

    void DemoState::setMessage(std::string text)
    {
        note = std::move(text);
    }

} // namespace antwika::ui_demo
