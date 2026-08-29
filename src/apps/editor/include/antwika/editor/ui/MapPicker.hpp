#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/widget/WidgetId.hpp>

namespace antwika::editor
{

    inline constexpr std::string_view kMapSuffix = ".json";

    inline constexpr std::size_t kMaxPicked = 24;

    inline constexpr std::size_t kMaxPickedRows = 2 * kMaxPicked;

    [[nodiscard]] widget::WidgetId getMapRowWidget(std::size_t rowIndex);

    [[nodiscard]] std::vector<std::string> getFilterMapNames(
        std::span<const std::string> names);

    [[nodiscard]] std::string getEnsureMapExtension(std::string_view name);

}
