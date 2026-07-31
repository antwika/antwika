#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::game
{

    using antwika::ui::WidgetId;

    /**
     * @brief Everything the save/load screen remembers between frames.
     *
     * **antwika::ui retains nothing**, deliberately: a field's
     * characters, its caret, a list's open flag and its selection and
     * which widget has focus are all handed *in* to a ui::Context and
     * handed back *out* of the frame it produces. So they live here,
     * where a replay regenerates them from the recorded key presses and
     * clicks exactly as it regenerates a camera -- see Camera.hpp for
     * the reason that matters and ui::Interactions::focused for the
     * library's side of it.
     *
     * **The list of saves is read once, at startup**, and this is where
     * it is kept for the run. Re-reading a directory inside the tick
     * path would let a live run and its replay see different options and
     * resolve one recorded click to two different files -- see
     * listSaveGames(). What the session itself writes is added through
     * add(), which is a consequence of a recorded click and so happens
     * identically on replay.
     *
     * Every name is held as a std::string and a std::string_view over it
     * is kept beside it, because ui::DropdownSpec borrows its options
     * and the strings would otherwise be rebuilt every frame.
     */
    class SaveLoadState final
    {
    public:
        /**
         * @brief Construct the screen's state over the saves that exist.
         * @param saves Every save's name, as listSaveGames() found them.
         * The first is selected when there is one, so pressing Load
         * without touching the list is the obvious thing rather than an
         * error.
         */
        explicit SaveLoadState(std::vector<std::string> saves = {});

        SaveLoadState(const SaveLoadState &) = delete;
        SaveLoadState(SaveLoadState &&) = delete;

        SaveLoadState &operator=(const SaveLoadState &) = delete;
        SaveLoadState &operator=(SaveLoadState &&) = delete;

        /**
         * @brief Get the options the picker lists.
         * @return A view per name, in the order they are shown.
         */
        [[nodiscard]] std::span<const std::string_view>
        options() const noexcept;

        /**
         * @brief Get which option is selected.
         * @return Its index, or ui::kNoOption when there is none.
         */
        [[nodiscard]] std::size_t selected() const noexcept;

        /**
         * @brief Get the selected save's name.
         * @return The name, or empty when nothing is selected.
         */
        [[nodiscard]] std::string_view selectedName() const noexcept;

        /**
         * @brief Select an option.
         * @param index The option to select; anything outside the list
         * selects nothing, which is what ui::kNoOption already means.
         */
        void select(std::size_t index) noexcept;

        /**
         * @brief Add a save the session has just written.
         *
         * Keeps the list sorted, so the order matches what
         * listSaveGames() would return next run, and selects the name
         * whether it was new or already there.
         *
         * @param name The save's name.
         */
        void add(const std::string &name);

        /**
         * @brief Check whether the picker's list is showing.
         * @return True while it is dropped down.
         */
        [[nodiscard]] bool listOpen() const noexcept;

        /**
         * @brief Show or hide the picker's list.
         * @param showing Whether to draw the options.
         */
        void setListOpen(bool showing) noexcept;

        /**
         * @brief Get what has been typed into the name field.
         * @return The characters, empty until something is typed.
         */
        [[nodiscard]] const std::string &name() const noexcept;

        /**
         * @brief Get where the name field's caret sits.
         * @return An index into name().
         */
        [[nodiscard]] std::size_t caret() const noexcept;

        /**
         * @brief Take an edit the UI reported.
         * @param text What the field's characters became.
         * @param cursor Where the caret ended up.
         */
        void setName(std::string text, std::size_t cursor);

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
         * @brief Get what the screen last said to whoever is looking.
         * @return The message, empty until something is said.
         */
        [[nodiscard]] const std::string &message() const noexcept;

        /**
         * @brief Say something to whoever is looking.
         * @param text What to say.
         */
        void setMessage(std::string text);

    private:
        void reindex();

        std::vector<std::string> names;
        std::vector<std::string_view> views;
        std::size_t chosen = antwika::ui::kNoOption;
        bool open = false;
        std::string typed;
        std::size_t cursor = antwika::ui::kCaretAtEnd;
        WidgetId focused = antwika::ui::kNoWidget;
        std::string note;
    };

} // namespace antwika::game
