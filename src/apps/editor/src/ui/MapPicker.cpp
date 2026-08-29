#include "antwika/editor/ui/MapPicker.hpp"

#include <cassert>
#include <cstdint>

#include <antwika/io/FileNames.hpp>

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    widget::WidgetId getMapRowWidget(const std::size_t rowIndex)
    {
        assert(rowIndex < kMaxPickedRows);

        return getWidgetAfter(kFirstMapRowWidget, rowIndex);
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
