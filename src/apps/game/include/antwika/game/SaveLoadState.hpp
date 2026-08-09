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

    class SaveLoadState final
    {
    public:
        explicit SaveLoadState(std::vector<std::string> saves = {});

        SaveLoadState(const SaveLoadState &) = delete;
        SaveLoadState(SaveLoadState &&) = delete;

        SaveLoadState &operator=(const SaveLoadState &) = delete;
        SaveLoadState &operator=(SaveLoadState &&) = delete;

        [[nodiscard]] std::span<const std::string_view>
        options() const noexcept;

        [[nodiscard]] std::size_t selected() const noexcept;

        [[nodiscard]] std::string_view selectedName() const noexcept;

        void select(std::size_t index) noexcept;

        void add(const std::string &name);

        [[nodiscard]] bool listOpen() const noexcept;

        void setListOpen(bool showing) noexcept;

        [[nodiscard]] const std::string &name() const noexcept;

        [[nodiscard]] std::size_t caret() const noexcept;

        void setName(std::string text, std::size_t cursor);

        [[nodiscard]] WidgetId focus() const noexcept;

        void setFocus(WidgetId id) noexcept;

        [[nodiscard]] const std::string &message() const noexcept;

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

}
