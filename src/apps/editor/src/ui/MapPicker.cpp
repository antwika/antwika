#include "antwika/editor/ui/MapPicker.hpp"

#include <cstdint>

#include <antwika/io/FileNames.hpp>

namespace antwika::editor
{

    widget::WidgetId getMapRowWidget(const std::size_t rowIndex)
    {
        return static_cast<widget::WidgetId>(
            static_cast<std::uint64_t>(kFirstMapRowWidget)
            + (rowIndex % kMaxPicked));
    }

    std::vector<std::string> getFilterMapNames(
        const std::span<const std::string> names)
    {
        return io::getFilteredBySuffix(names, kMapSuffix, kMaxPicked);
    } // GCOVR_EXCL_LINE

    std::string getEnsureMapExtension(const std::string_view name)
    {
        return io::getWithSuffix(name, kMapSuffix);
    } // GCOVR_EXCL_LINE

}
