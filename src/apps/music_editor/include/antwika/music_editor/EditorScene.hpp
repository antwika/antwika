#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    /**
     * @brief What the status line says about the sound.
     *
     * Handed in rather than read, because everything in it belongs to
     * the projection side and a scene that reached for it would be
     * reading the audio back into the picture.
     */
    struct PlaybackStatus
    {
        std::uint64_t started = 0;
        std::size_t voices = 0;
        std::uint64_t cycles = 0;
    };

    /**
     * @brief Get what a track is called.
     * @param track Which track.
     * @return Its name, which outlives every caller.
     */
    [[nodiscard]] std::string_view trackName(std::size_t track) noexcept;

    /**
     * @brief The whole editor: four lines, what they say, and two
     * buttons.
     *
     * Stateless and deterministic, exactly as ui_demo::DemoScene is: the
     * same canvas, pointer, keyboard and state always describe the same
     * picture and answer the same way about what was pressed -- which is
     * what lets the entire layout be asserted with `EXPECT_EQ` and no
     * window, no mock and no backend.
     *
     * It holds none of the lines, the caret or the focus.
     * Those arrive in the state and go back out through the frame,
     * because antwika::ui retains nothing between frames and neither may
     * this.
     *
     * **Its words are fixed English literals rather than an
     * i18n::Translator.**
     * A field is as wide as the label beside it, so the layout is a
     * function of the words, and a session recorded in one language and
     * replayed in another would resolve a click to a different widget.
     * ui_demo answers that by fixing the locale in `main()`; this app
     * answers it by having one language, which is the same guarantee
     * with nothing to configure -- and the words that matter here are
     * the mini-notation, which is not English in the first place.
     */
    class EditorScene final
    {
    public:
        /**
         * @brief Describe the editor as it now stands.
         * @param state The lines, the focus and the caret.
         * @param score What each line currently parses into.
         * @param status What to say about the sound.
         * @param canvas The size the window was **asked** for.
         * @param pointer Where the pointer is and what it is doing.
         * @param keyboard What arrived this frame.
         * @return The picture and what the input did to it.
         */
        [[nodiscard]] Frame describe(
            const EditorState &state,
            const Score &score,
            const PlaybackStatus &status,
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard) const;

        /**
         * @brief Draw a described picture.
         * @param renderer Where to draw.
         * @param picture What describe() produced.
         */
        void draw(IRenderer &renderer, const DrawList &picture) const;
    };

} // namespace antwika::music_editor
