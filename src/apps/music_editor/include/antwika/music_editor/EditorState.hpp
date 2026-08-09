#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ui/ScrollChange.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/TextEdit.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/music_editor/EditorKeys.hpp"

namespace antwika::music_editor
{

    using antwika::ui::WidgetId;

    inline constexpr WidgetId kCodeField{1};

    inline constexpr WidgetId kPlayButton{100};

    inline constexpr WidgetId kPanicButton{101};

    inline constexpr WidgetId kLayoutBox{102};

    inline constexpr WidgetId kMenuBox{103};

    inline constexpr WidgetId kSpeedBox{104};

    inline constexpr WidgetId kSaveNameField{110};

    inline constexpr WidgetId kSaveConfirm{111};

    inline constexpr WidgetId kModalCancel{112};

    inline constexpr WidgetId kLayoutOptions{200};

    inline constexpr WidgetId kMenuOptions{300};

    inline constexpr WidgetId kLoadOptions{400};

    inline constexpr WidgetId kSpeedOptions{500};

    inline constexpr WidgetId kPianorollBands{600};

    inline constexpr WidgetId kWaveformBands{700};

    struct SpeedChoice final
    {
        std::string_view label;

        std::int64_t numerator;

        std::int64_t denominator;
    };

    inline constexpr std::array<SpeedChoice, 5> kSpeeds{
        SpeedChoice{.label = "0.25x", .numerator = 1, .denominator = 4},
        SpeedChoice{.label = "0.5x", .numerator = 1, .denominator = 2},
        SpeedChoice{.label = "1x", .numerator = 1, .denominator = 1},
        SpeedChoice{.label = "2x", .numerator = 2, .denominator = 1},
        SpeedChoice{.label = "4x", .numerator = 4, .denominator = 1}};

    inline constexpr std::size_t kNormalSpeed = 2;

    [[nodiscard]] constexpr WidgetId loadOption(
        const std::size_t at) noexcept
    {
        return WidgetId{
            static_cast<std::uint64_t>(kLoadOptions) + at};
    }

    [[nodiscard]] constexpr WidgetId pianorollBand(
        const std::size_t at) noexcept
    {
        return WidgetId{
            static_cast<std::uint64_t>(kPianorollBands) + at};
    }

    [[nodiscard]] constexpr WidgetId waveformBand(
        const std::size_t at) noexcept
    {
        return WidgetId{
            static_cast<std::uint64_t>(kWaveformBands) + at};
    }

    enum class Modal : std::uint8_t
    {
        None = 0,

        Save,

        Load,
    };

    [[nodiscard]] constexpr Modal enumBound(Modal) noexcept
    {
        return Modal::Load;
    }

    struct EditorState final
    {
        std::string source{};

        std::size_t cursor = ui::kCaretAtEnd;

        std::optional<std::size_t> anchor{};

        std::size_t scroll = 0;

        std::string clipboard{};

        KeyLayout layout = KeyLayout::Swedish;

        bool layoutOpen = false;

        ui::DragHome dragging = ui::DragHome::None;

        bool paused = false;

        bool menuOpen = false;

        std::size_t speed = kNormalSpeed;

        bool speedOpen = false;

        Modal modal = Modal::None;

        std::string fileName{};

        std::size_t fileCursor = ui::kCaretAtEnd;

        std::string notice{};

        std::vector<std::string> scores{};

        [[nodiscard]] bool operator==(const EditorState &other) const
            = default;
    };

    [[nodiscard]] EditorState openingState();

    void applyEdit(EditorState &state, const ui::TextEdit &edit);

    void addScore(EditorState &state, const std::string &name);

    void applyScroll(EditorState &state, const ui::ScrollChange &scrolled);

}
