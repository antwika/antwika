#include "antwika/editor/editor/state/CharacterTool.hpp"

#include <utility>

namespace antwika::editor
{

    std::optional<std::size_t> CharacterTool::getChosenCharacter(
        const std::size_t characterCount) const noexcept
    {
        if (!chosenIndex.has_value() || *chosenIndex >= characterCount)
        {
            return std::nullopt;
        }

        return chosenIndex;
    }

    bool CharacterTool::hasChoice() const noexcept
    {
        return chosenIndex.has_value();
    }

    bool CharacterTool::isChosen(const std::size_t index) const noexcept
    {
        return chosenIndex == index;
    }

    void CharacterTool::choose(const std::size_t index) noexcept
    {
        chosenIndex = index;
        placed = false;
        resetInspector();
    }

    void CharacterTool::dropChoice() noexcept
    {
        chosenIndex.reset();
        resetInspector();
    }

    bool CharacterTool::isPlaced() const noexcept
    {
        return placed;
    }

    void CharacterTool::markPlaced() noexcept
    {
        placed = true;
    }

    const std::string &CharacterTool::getPendingLine() const noexcept
    {
        return pendingLine;
    }

    void CharacterTool::setPendingLine(std::string line)
    {
        pendingLine = std::move(line);
    }

    std::string CharacterTool::takePendingLine()
    {
        return std::exchange(pendingLine, {});
    }

    std::optional<std::size_t> CharacterTool::getOpenComponent()
        const noexcept
    {
        return openComponent;
    }

    void CharacterTool::toggleComponent(const std::size_t slot) noexcept
    {
        openComponent = openComponent == slot
                            ? std::optional<std::size_t>{}
                            : std::optional{slot};
    }

    void CharacterTool::closeComponent() noexcept
    {
        openComponent.reset();
    }

    bool CharacterTool::isAddListOpen() const noexcept
    {
        return addListOpen;
    }

    void CharacterTool::toggleAddList() noexcept
    {
        addListOpen = !addListOpen;
    }

    void CharacterTool::closeAddList() noexcept
    {
        addListOpen = false;
    }

    const std::string &CharacterTool::getPendingValueText() const noexcept
    {
        return pendingValueText;
    }

    void CharacterTool::setPendingValueText(std::string text)
    {
        pendingValueText = std::move(text);
    }

    std::optional<widget::WidgetId> CharacterTool::getEditingValueWidget()
        const noexcept
    {
        return editingValueWidget;
    }

    void CharacterTool::beginValueEdit(
        const widget::WidgetId widgetId, std::string text)
    {
        editingValueWidget = widgetId;
        pendingValueText = std::move(text);
    }

    void CharacterTool::endValueEdit() noexcept
    {
        editingValueWidget.reset();
        pendingValueText.clear();
    }

    std::size_t CharacterTool::getInspectorScroll() const noexcept
    {
        return inspectorScroll;
    }

    void CharacterTool::setInspectorScroll(const std::size_t offset) noexcept
    {
        inspectorScroll = offset;
    }

    bool CharacterTool::isInspectorTrackHeld() const noexcept
    {
        return inspectorTrackHeld;
    }

    void CharacterTool::setInspectorTrackHeld(const bool held) noexcept
    {
        inspectorTrackHeld = held;
    }

    void CharacterTool::resetInspector() noexcept
    {
        openComponent.reset();
        addListOpen = false;
        endValueEdit();
        inspectorScroll = 0;
        inspectorTrackHeld = false;
    }

}
