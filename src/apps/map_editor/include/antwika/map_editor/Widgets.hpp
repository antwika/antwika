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

    inline constexpr ui::WidgetId kLevelUp{10};

    inline constexpr ui::WidgetId kLevelDown{11};

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

    inline constexpr std::uint64_t kRulesPairBase = 700;

    inline constexpr std::size_t kRulesTerrains = 6;

    [[nodiscard]] constexpr ui::WidgetId rulesPairButton(
        const std::size_t row, const std::size_t column) noexcept
    {
        return static_cast<ui::WidgetId>(
            kRulesPairBase + row * kRulesTerrains + column);
    }

    [[nodiscard]] constexpr std::optional<std::size_t> rulesPairIndex(
        const ui::WidgetId id) noexcept
    {
        const auto raw = static_cast<std::uint64_t>(id);

        if (raw < kRulesPairBase
            || raw >= kRulesPairBase
                          + kRulesTerrains * kRulesTerrains)
        {
            return std::nullopt;
        }

        return raw - kRulesPairBase;
    }

    inline constexpr std::uint64_t kRulesWeightDownBase = 740;

    inline constexpr std::uint64_t kRulesWeightUpBase = 750;

    inline constexpr ui::WidgetId kRulesApply{760};

    inline constexpr ui::WidgetId kRulesCancel{761};

    inline constexpr ui::WidgetId kDrawInk{790};

    inline constexpr ui::WidgetId kDrawPaper{791};

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

    inline constexpr ui::WidgetId kTilesetPicker{1000};

    inline constexpr std::uint64_t kTilesetOptionBase = 1010;

    inline constexpr std::size_t kTilesetOptionCount = 64;

    [[nodiscard]] constexpr ui::WidgetId tilesetOption(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kTilesetOptionBase + index);
    }

    inline constexpr ui::WidgetId kToolDraw{1100};

    inline constexpr ui::WidgetId kToolSockets{1101};

    inline constexpr ui::WidgetId kToolDecor{1102};

    inline constexpr ui::WidgetId kToolSelect{1103};

    inline constexpr std::uint64_t kFrameButtonBase = 1110;

    inline constexpr std::size_t kFrameButtonCount = 4;

    [[nodiscard]] constexpr ui::WidgetId frameButton(
        const std::size_t frame) noexcept
    {
        return static_cast<ui::WidgetId>(kFrameButtonBase + frame);
    }

    inline constexpr ui::WidgetId kFrameClear{1114};

    inline constexpr ui::WidgetId kLayerAdd{1120};

    inline constexpr ui::WidgetId kLayerRemove{1121};

    inline constexpr std::uint64_t kLayerRowBase = 1130;

    inline constexpr std::size_t kLayerRowCount = 8;

    [[nodiscard]] constexpr ui::WidgetId layerRow(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kLayerRowBase + index);
    }

    inline constexpr ui::WidgetId kSpriteAdd{1140};

    inline constexpr ui::WidgetId kSpriteDuplicate{1141};

    inline constexpr ui::WidgetId kSpriteDelete{1142};

    inline constexpr ui::WidgetId kSocketName{1150};

    inline constexpr ui::WidgetId kSocketAdd{1151};

    inline constexpr ui::WidgetId kSocketRename{1152};

    inline constexpr ui::WidgetId kSocketDelete{1153};

    inline constexpr std::uint64_t kSocketRowBase = 1160;

    inline constexpr std::size_t kSocketRowCount = 14;

    [[nodiscard]] constexpr ui::WidgetId socketRow(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kSocketRowBase + index);
    }

    inline constexpr ui::WidgetId kDecorAll{1190};

    inline constexpr ui::WidgetId kDecorNone{1191};

    inline constexpr ui::WidgetId kDensityDown{1194};

    inline constexpr ui::WidgetId kDensityValue{1195};

    inline constexpr ui::WidgetId kDensityUp{1196};

    inline constexpr ui::WidgetId kWeightDown{1197};

    inline constexpr ui::WidgetId kWeightValue{1198};

    inline constexpr ui::WidgetId kWeightUp{1199};

    inline constexpr ui::WidgetId kNewTilesetName{1200};

    inline constexpr ui::WidgetId kNewTilesetTerrain{1201};

    inline constexpr ui::WidgetId kNewTilesetCreate{1202};

    inline constexpr ui::WidgetId kNewTilesetCancel{1203};

    inline constexpr std::uint64_t kNewTilesetTerrainBase = 1210;

    inline constexpr ui::WidgetId kBindingsApply{1220};

    inline constexpr ui::WidgetId kBindingsCancel{1221};

    inline constexpr std::uint64_t kBindingPickerBase = 1230;

    [[nodiscard]] constexpr ui::WidgetId bindingPicker(
        const std::size_t terrain) noexcept
    {
        return static_cast<ui::WidgetId>(kBindingPickerBase + terrain);
    }

    inline constexpr std::uint64_t kBindingOptionBase = 1240;

    inline constexpr std::size_t kBindingOptionStride = 32;

    [[nodiscard]] constexpr ui::WidgetId bindingOption(
        const std::size_t terrain) noexcept
    {
        return static_cast<ui::WidgetId>(
            kBindingOptionBase + terrain * kBindingOptionStride);
    }

    inline constexpr std::uint64_t kKeysRowBase = 1500;

    [[nodiscard]] constexpr ui::WidgetId keysRow(
        const std::size_t index) noexcept
    {
        return static_cast<ui::WidgetId>(kKeysRowBase + index);
    }

    inline constexpr ui::WidgetId kKeysDefaults{1540};

    inline constexpr ui::WidgetId kKeysClose{1541};

    inline constexpr ui::WidgetId kPickerToggle{1600};

    inline constexpr ui::WidgetId kMapSelectTool{1601};

    inline constexpr ui::WidgetId kCharToolDraw{1610};

    inline constexpr ui::WidgetId kCharToolSelect{1611};

    [[nodiscard]] constexpr std::optional<std::size_t> rangeIndex(
        const ui::WidgetId id,
        const std::uint64_t base,
        const std::size_t count) noexcept
    {
        const auto raw = static_cast<std::uint64_t>(id);

        if (raw < base || raw >= base + count)
        {
            return std::nullopt;
        }

        return raw - base;
    }

    [[nodiscard]] constexpr bool isField(const ui::WidgetId id) noexcept
    {
        return id == kFieldId || id == kFieldTargetMap
               || id == kFieldTargetEntry || id == kFieldTags
               || id == kDialogName || id == kCharName
               || id == kPaletteHex || id == kSocketName
               || id == kNewTilesetName;
    }

}
