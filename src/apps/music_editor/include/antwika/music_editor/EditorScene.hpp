#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/sequencer/Rational.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/TextAreaSpec.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"
#include "antwika/music_editor/WaveImage.hpp"

namespace antwika::music_editor
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    inline constexpr std::uint32_t kTextScale = 2;

    inline constexpr std::uint32_t kPianorollRows = 3;

    inline constexpr std::uint32_t kPianorollWidth = 640;

    inline constexpr std::uint32_t kWaveformRows = 3;

    [[nodiscard]] antwika::ui::Theme editorTheme() noexcept;

    struct PlaybackStatus final
    {
        std::uint64_t started = 0;
        std::size_t voices = 0;
        std::uint64_t cycles = 0;

        std::size_t lines = 0;

        std::vector<antwika::ui::TextHighlight> playing{};

        sequencer::Rational position{};

        SampleRate rate = 0;

        std::int64_t cycleFrames = 0;

        std::span<const WaveImage> waves{};
    };

    class EditorScene final
    {
    public:
        [[nodiscard]] Frame describe(
            const EditorState &state,
            const Score &score,
            const PlaybackStatus &status,
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard) const;

        [[nodiscard]] Frame describeModal(
            const EditorState &state,
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard) const;

        void draw(IRenderer &renderer, const DrawList &picture) const;
    };

}
