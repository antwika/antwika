#include "antwika/atlas_editor/Tool.hpp"

#include <array>
#include <cstddef>
#include <string_view>

#include "antwika/atlas_editor/MessageId.hpp"

namespace antwika::atlas_editor
{

    MessageId toolNameId(const Tool tool) noexcept
    {
        constexpr std::array<MessageId, kToolCount> names{
            MessageId::ToolPaint,
            MessageId::ToolErase,
            MessageId::ToolFill,
            MessageId::ToolPick,
            MessageId::ToolSelect,
            MessageId::ToolLine,
            MessageId::ToolEllipse};

        const auto index = static_cast<std::size_t>(tool);

        if (index >= names.size())
        {
            return names.front();
        }

        return names[index];
    }

    std::string_view toolMark(const Tool tool) noexcept
    {
        constexpr std::array<std::string_view, kToolCount> marks{
            "Pa", "Er", "Fi", "Pi", "Se", "Li", "El"};

        const auto index = static_cast<std::size_t>(tool);

        if (index >= marks.size())
        {
            return marks.front();
        }

        return marks[index];
    }

}
