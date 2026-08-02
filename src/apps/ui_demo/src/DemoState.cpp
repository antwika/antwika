#include "antwika/ui_demo/DemoState.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace antwika::ui_demo
{

    namespace
    {
        constexpr std::size_t kOpeningLines = 12;

        // Enough lines that the pane scrolls from the first frame.
        [[nodiscard]] std::string openingPane()
        {
            std::string text;

            for (std::size_t line = 0; line < kOpeningLines; ++line)
            {
                text += "line " + std::to_string(line);

                if (line + 1 < kOpeningLines)
                {
                    text += '\n';
                }
            }

            return text;
            // Only an unwind destroys text at this brace.
        } // GCOVR_EXCL_LINE
    } // namespace

    DemoState::DemoState() : paneText(openingPane())
    {
    }

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

    const std::optional<DemoMessage> &DemoState::message()
        const noexcept
    {
        return note;
    }

    void DemoState::setMessage(DemoMessage text)
    {
        note = std::move(text);
    }

    const std::string &DemoState::areaText() const noexcept
    {
        return paneText;
    }

    std::size_t DemoState::areaCursor() const noexcept
    {
        return paneCursor;
    }

    std::size_t DemoState::areaAnchor() const noexcept
    {
        return paneAnchor;
    }

    void DemoState::setArea(
        std::string characters,
        const std::size_t at,
        const std::size_t other)
    {
        paneText = std::move(characters);
        paneCursor = at;
        paneAnchor = other;
    }

    std::size_t DemoState::areaScroll() const noexcept
    {
        return paneScroll;
    }

    void DemoState::setAreaScroll(const std::size_t line)
    {
        paneScroll = line;
    }

} // namespace antwika::ui_demo
