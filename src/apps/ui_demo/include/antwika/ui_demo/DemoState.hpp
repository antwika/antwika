#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/ui_demo/DemoMessage.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Showcase.hpp"

namespace antwika::ui_demo
{

    using antwika::ui::WidgetId;

    /**
     * @brief How many accents there are.
     */
    inline constexpr std::size_t kAccentCount = 3;

    /**
     * @brief Get which message names one accent.
     *
     * An id rather than the words, following showcaseNameId().
     *
     * @param index Which accent, in the order the list shows them.
     * @return Its message id, or the first accent's for an index the
     * list does not offer -- the same fall-back-to-the-first rule
     * atlas_editor::toolNameId() follows, and for the same reason.
     */
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

    /**
     * @brief Everything the showcase remembers between frames.
     *
     * **antwika::ui retains nothing**, deliberately: which page is
     * shown, whether either list is open, what either has selected, a
     * field's characters and caret, and which widget has focus are all
     * handed *in* to a ui::Context and handed back *out* of the frame it
     * produces.
     * So they live here, where a replay regenerates them from the
     * recorded clicks and key presses rather than from anything the
     * library remembered -- see ui::Interactions::focused for the
     * library's side of the same argument.
     *
     * That is what makes the picker at the top of this demo worth
     * having: it is one of the elements being shown, and the bit saying
     * whether it is dropped down is this object's.
     */
    class DemoState final
    {
    public:
        /**
         * @brief Construct the state a fresh demo opens with.
         *
         * The pane opens on a few numbered lines rather than empty,
         * so its scrollbar has something to say from the first frame.
         */
        DemoState();

        /**
         * @brief Get which page is being shown.
         * @return The page; the first one until something chooses.
         */
        [[nodiscard]] Showcase showcase() const noexcept;

        /**
         * @brief Get which of the picker's options is selected.
         * @return Its index, which always names a page.
         */
        [[nodiscard]] std::size_t selected() const noexcept;

        /**
         * @brief Show a page.
         * @param index The option to select; anything outside the list
         * leaves the page where it was, since a page nobody wrote is not
         * one this demo can draw.
         */
        void select(std::size_t index) noexcept;

        /**
         * @brief Check whether the picker's list is showing.
         * @return True while it is dropped down.
         */
        [[nodiscard]] bool pickerOpen() const noexcept;

        /**
         * @brief Show or hide the picker's list.
         * @param showing Whether to draw the options.
         */
        void setPickerOpen(bool showing) noexcept;

        /**
         * @brief Get which accent is selected.
         * @return Its index, or ui::kNoOption while there is none.
         */
        [[nodiscard]] std::size_t accent() const noexcept;

        /**
         * @brief Choose an accent.
         * @param index The option to select; anything outside the list
         * selects nothing, which is what ui::kNoOption already means.
         */
        void selectAccent(std::size_t index) noexcept;

        /**
         * @brief Check whether the accent list is showing.
         * @return True while it is dropped down.
         */
        [[nodiscard]] bool accentOpen() const noexcept;

        /**
         * @brief Show or hide the accent list.
         * @param showing Whether to draw the options.
         */
        void setAccentOpen(bool showing) noexcept;

        /**
         * @brief Get what has been typed into the field.
         * @return The characters, empty until something is typed.
         */
        [[nodiscard]] const std::string &text() const noexcept;

        /**
         * @brief Get where the field's caret sits.
         * @return An index into text().
         */
        [[nodiscard]] std::size_t caret() const noexcept;

        /**
         * @brief Take an edit the UI reported.
         * @param characters What the field's characters became.
         * @param at Where the caret ended up.
         */
        void setText(std::string characters, std::size_t at);

        /**
         * @brief Get which widget the keyboard is on.
         * @return The focused widget, or ui::kNoWidget.
         */
        [[nodiscard]] WidgetId focus() const noexcept;

        /**
         * @brief Take the focus the last frame handed back.
         * @param id The widget the keyboard is now on.
         */
        void setFocus(WidgetId id) noexcept;

        /**
         * @brief Get how many times the counting button was pressed.
         * @return The count, which the reset button puts back to zero.
         */
        [[nodiscard]] std::uint32_t clicks() const noexcept;

        /**
         * @brief Count one press of the counting button.
         */
        void countClick() noexcept;

        /**
         * @brief Put the click counter back to zero.
         */
        void resetClicks() noexcept;

        /**
         * @brief Get what the demo last said about what happened.
         * @return The message, or nothing until something is said.
         */
        [[nodiscard]] const std::optional<DemoMessage> &message()
            const noexcept;

        /**
         * @brief Say what just happened.
         * @param text Which message, and what it names.
         */
        void setMessage(DemoMessage text);

        /**
         * @brief Get the many-line pane's whole document.
         * @return The characters, a few numbered lines to start.
         */
        [[nodiscard]] const std::string &areaText() const noexcept;

        /**
         * @brief Get where the pane's caret sits.
         * @return An index into areaText().
         */
        [[nodiscard]] std::size_t areaCursor() const noexcept;

        /**
         * @brief Get where the pane's selection ends, if one does.
         * @return The other end's index; the caret's when none.
         */
        [[nodiscard]] std::size_t areaAnchor() const noexcept;

        /**
         * @brief Take an edit the pane reported.
         * @param characters What the document became.
         * @param at Where the caret ended up.
         * @param other Where the selection's far end ended up.
         */
        void setArea(
            std::string characters, std::size_t at, std::size_t other);

        /**
         * @brief Get which line the pane shows at its top.
         * @return The line, handed back from the last frame's report.
         */
        [[nodiscard]] std::size_t areaScroll() const noexcept;

        /**
         * @brief Take the line the pane says it is showing.
         * @param line What Interactions::scrolled reported.
         */
        void setAreaScroll(std::size_t line);

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
    };

} // namespace antwika::ui_demo
