#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    class CharacterTool final
    {
    public:
        [[nodiscard]] std::optional<std::size_t> getChosenCharacter(
            std::size_t characterCount) const noexcept;

        [[nodiscard]] bool hasChoice() const noexcept;

        [[nodiscard]] bool isChosen(std::size_t index) const noexcept;

        void choose(std::size_t index) noexcept;

        void dropChoice() noexcept;

        [[nodiscard]] bool isPlaced() const noexcept;

        void markPlaced() noexcept;

        [[nodiscard]] const std::string &getPendingLine() const noexcept;

        void setPendingLine(std::string line);

        [[nodiscard]] std::string takePendingLine();

        [[nodiscard]] std::optional<std::size_t> getOpenComponent()
            const noexcept;

        void toggleComponent(std::size_t slot) noexcept;

        void closeComponent() noexcept;

        [[nodiscard]] bool isAddListOpen() const noexcept;

        void toggleAddList() noexcept;

        void closeAddList() noexcept;

        [[nodiscard]] const std::string &getPendingValueText()
            const noexcept;

        void setPendingValueText(std::string text);

        [[nodiscard]] std::optional<widget::WidgetId>
        getEditingValueWidget() const noexcept;

        void beginValueEdit(widget::WidgetId widgetId, std::string text);

        void endValueEdit() noexcept;

        [[nodiscard]] std::size_t getInspectorScroll() const noexcept;

        void setInspectorScroll(std::size_t offset) noexcept;

        [[nodiscard]] bool isInspectorTrackHeld() const noexcept;

        void setInspectorTrackHeld(bool held) noexcept;

    private:
        void resetInspector() noexcept;

        std::optional<std::size_t> chosenIndex;

        bool placed = false;

        std::string pendingLine;

        std::optional<std::size_t> openComponent;

        bool addListOpen = false;

        std::string pendingValueText;

        std::optional<widget::WidgetId> editingValueWidget;

        std::size_t inspectorScroll = 0;

        bool inspectorTrackHeld = false;
    };

}
