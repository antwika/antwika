#include "antwika/atlas_editor/Tool.hpp"

#include <array>
#include <cstddef>

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
            MessageId::ToolSelect};

        const auto index = static_cast<std::size_t>(tool);

        // A Tool is a scoped enum.
        // A value outside the enumerators can only come from a cast.
        // A cast is exactly what the toolbar walks the tools with.
        // So the guard is cheaper than trusting every caller to count.
        if (index >= names.size())
        {
            return names.front();
        }

        return names[index];
    }

} // namespace antwika::atlas_editor
