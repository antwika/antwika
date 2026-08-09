#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoMessage.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Showcase.hpp"

namespace antwika::ui_demo
{

    using antwika::ui::WidgetId;

    inline constexpr std::size_t kAccentCount = 3;

    [[nodiscard]] constexpr MessageId accentNameId(
        const std::size_t index) noexcept
    {
        constexpr std::array<MessageId, kAccentCount>
            ids{
                MessageId::AccentAmber,
                MessageId::AccentMint,
                MessageId::AccentRose};

        return ids[index % kAccentCount];
    }

    class DemoState final
    {
    public:
        DemoState();

        [[nodiscard]] Showcase showcase() const noexcept;

        [[nodiscard]] std::size_t selected() const noexcept;

        void select(std::size_t index) noexcept;

        [[nodiscard]] bool pickerOpen() const noexcept;

        void setPickerOpen(bool showing) noexcept;

        [[nodiscard]] std::size_t accent() const noexcept;

        void selectAccent(std::size_t index) noexcept;

        [[nodiscard]] bool accentOpen() const noexcept;

        void setAccentOpen(bool showing) noexcept;

        [[nodiscard]] const std::string &text() const noexcept;

        [[nodiscard]] std::size_t caret() const noexcept;

        void setText(std::string characters, std::size_t at);

        [[nodiscard]] WidgetId focus() const noexcept;

        void setFocus(WidgetId id) noexcept;

        [[nodiscard]] std::uint32_t clicks() const noexcept;

        void countClick() noexcept;

        void resetClicks() noexcept;

        [[nodiscard]] const std::optional<DemoMessage> &message()
            const noexcept;

        void setMessage(DemoMessage text);

        [[nodiscard]] const std::string &areaText() const noexcept;

        [[nodiscard]] std::size_t areaCursor() const noexcept;

        [[nodiscard]] std::size_t areaAnchor() const noexcept;

        void setArea(
            std::string characters, std::size_t at, std::size_t other);

        [[nodiscard]] std::size_t areaScroll() const noexcept;

        void setAreaScroll(std::size_t line);

        [[nodiscard]] antwika::ui::DragHome areaDragging()
            const noexcept;

        void setAreaDragging(antwika::ui::DragHome home) noexcept;

    private:
        Showcase page = Showcase::Labels;
        bool pickerShowing = false;
        std::size_t chosenAccent = antwika::ui::kNoOption;
        bool accentShowing = false;
        std::string typed;
        std::size_t cursor = antwika::ui::kCaretAtEnd;
        WidgetId focused = antwika::ui::kNoWidget;
        std::uint32_t clickCount = 0;
        std::optional<DemoMessage> note;

        std::string paneText;
        std::size_t paneCursor = 0;
        std::size_t paneAnchor = 0;
        std::size_t paneScroll = 0;
        antwika::ui::DragHome paneDragging =
            antwika::ui::DragHome::None;
    };

}
