#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include <antwika/ui/WidgetId.hpp>

namespace antwika::map_editor::widgets
{

    inline constexpr std::uint64_t kTerrainBase = 1;

    inline constexpr std::size_t kPaletteCount = 7;

    inline constexpr std::size_t kFreeBrushIndex = 6;

    [[nodiscard]] constexpr ui::WidgetId terrainButton(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kTerrainBase + index);
    }

    inline constexpr ui::WidgetId kHeightUp{10};

    inline constexpr ui::WidgetId kHeightDown{11};

    inline constexpr ui::WidgetId kBridge{12};

    inline constexpr ui::WidgetId kLight{13};

    inline constexpr ui::WidgetId kGenerate{14};

    inline constexpr ui::WidgetId kKindPicker{40};

    inline constexpr ui::WidgetId kKindFirst{41};

    inline constexpr ui::WidgetId kPlace{50};

    inline constexpr ui::WidgetId kDelete{51};

    inline constexpr ui::WidgetId kFieldId{60};

    inline constexpr ui::WidgetId kFieldTargetMap{61};

    inline constexpr ui::WidgetId kFieldTargetEntry{62};

    inline constexpr ui::WidgetId kFieldTags{63};

    inline constexpr std::uint64_t kMenuBase = 70;

    inline constexpr std::size_t kMenuCount = 4;

    [[nodiscard]] constexpr ui::WidgetId menuTitle(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kMenuBase + index);
    }

    [[nodiscard]] constexpr std::optional<std::size_t> menuIndexOf(
        const ui::WidgetId id) noexcept
    {
        const auto raw = static_cast<std::uint64_t>(id);

        if (raw < kMenuBase || raw >= kMenuBase + kMenuCount)
        {
            return std::nullopt;
        }

        return raw - kMenuBase;
    }

    inline constexpr ui::WidgetId kMenuFileFirst{80};

    inline constexpr ui::WidgetId kMenuEditFirst{90};

    inline constexpr ui::WidgetId kMenuViewFirst{100};

    inline constexpr ui::WidgetId kMenuMapFirst{110};

    inline constexpr ui::WidgetId kDialogPrev{201};

    inline constexpr ui::WidgetId kDialogNext{202};

    inline constexpr ui::WidgetId kDialogConfirm{203};

    inline constexpr ui::WidgetId kDialogCancel{204};

    inline constexpr ui::WidgetId kDialogName{210};

    inline constexpr std::uint64_t kDialogRowBase = 220;

    [[nodiscard]] constexpr ui::WidgetId dialogRow(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kDialogRowBase + index);
    }

    [[nodiscard]] constexpr std::optional<std::size_t> dialogRowIndex(
        const ui::WidgetId id, const std::size_t rows) noexcept
    {
        const auto raw = static_cast<std::uint64_t>(id);

        if (raw < kDialogRowBase || raw >= kDialogRowBase + rows)
        {
            return std::nullopt;
        }

        return raw - kDialogRowBase;
    }

    inline constexpr ui::WidgetId kPaletteSwatchInk{240};

    inline constexpr ui::WidgetId kPaletteSwatchPaper{241};

    inline constexpr ui::WidgetId kPaletteHue{242};

    inline constexpr ui::WidgetId kPaletteSv{243};

    inline constexpr ui::WidgetId kPaletteHex{244};

    inline constexpr ui::WidgetId kPaletteApply{245};

    inline constexpr ui::WidgetId kPaletteCancel{246};

    inline constexpr ui::WidgetId kCharName{300};

    inline constexpr ui::WidgetId kCharNew{301};

    inline constexpr ui::WidgetId kCharDelete{302};

    inline constexpr ui::WidgetId kEnemyPicker{310};

    inline constexpr ui::WidgetId kEnemyFirst{400};

    inline constexpr std::uint64_t kCharRowBase = 500;

    [[nodiscard]] constexpr ui::WidgetId characterRow(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kCharRowBase + index);
    }

    [[nodiscard]] constexpr std::optional<std::size_t> characterRowIndex(
        const ui::WidgetId id, const std::size_t rows) noexcept
    {
        const auto raw = static_cast<std::uint64_t>(id);

        if (raw < kCharRowBase || raw >= kCharRowBase + rows)
        {
            return std::nullopt;
        }

        return raw - kCharRowBase;
    }

    [[nodiscard]] constexpr bool isField(const ui::WidgetId id) noexcept
    {
        return id == kFieldId || id == kFieldTargetMap
               || id == kFieldTargetEntry || id == kFieldTags
               || id == kDialogName || id == kCharName
               || id == kPaletteHex;
    }

}
