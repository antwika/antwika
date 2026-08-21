#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ui/WidgetId.hpp>

namespace antwika::editor
{

    inline constexpr std::string_view kMapSuffix = ".json";

    inline constexpr ui::WidgetId kPickerNameWidget{240};

    inline constexpr ui::WidgetId kPickerConfirmWidget{241};

    inline constexpr ui::WidgetId kPickerCancelWidget{242};

    inline constexpr ui::WidgetId kPickerOverwriteWidget{243};

    inline constexpr ui::WidgetId kFirstMapRowWidget{256};

    inline constexpr std::size_t kMaxPicked = 24;

    [[nodiscard]] ui::WidgetId mapRowWidget(std::size_t rowIndex);

    [[nodiscard]] std::vector<std::string> filterMapNames(
        std::span<const std::string> names);

    [[nodiscard]] std::string ensureMapExtension(std::string_view name);

}
