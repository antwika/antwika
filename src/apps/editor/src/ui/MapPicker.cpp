#include "antwika/editor/ui/MapPicker.hpp"

#include <cstdint>

#include <antwika/io/FileNames.hpp>

namespace antwika::editor
{

    ui::WidgetId mapRowWidget(const std::size_t rowIndex)
    {
        return static_cast<ui::WidgetId>(
            static_cast<std::uint64_t>(kFirstMapRowWidget)
            + (rowIndex % kMaxPicked));
    }

    std::vector<std::string> filterMapNames(
        const std::span<const std::string> names)
    {
        return io::filteredBySuffix(names, kMapSuffix, kMaxPicked);
    } // GCOVR_EXCL_LINE

    std::string ensureMapExtension(const std::string_view name)
    {
        return io::withSuffix(name, kMapSuffix);
    } // GCOVR_EXCL_LINE

}
