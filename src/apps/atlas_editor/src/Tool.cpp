#include "antwika/atlas_editor/Tool.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::atlas_editor
{

    std::string_view toolName(const Tool tool) noexcept
    {
        constexpr std::array<std::string_view, kToolCount> names{
            "PAINT", "ERASE", "PICK"};

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
