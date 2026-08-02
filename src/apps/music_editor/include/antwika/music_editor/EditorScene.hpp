#pragma once

#include <cstddef>
#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/Theme.hpp>

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
     * @brief How much bigger than one glyph pixel the text is drawn.
     *
     * Twice, because what is being read here is code: a mis-read
     * bracket is a line that will not play, and this editor is looked
     * at for longer at a stretch than any dialog in the tree.
     */
    inline constexpr std::uint32_t kTextScale = 2;

    /**
     * @brief Get the theme the editor draws with.
     * @return The theme, which is the default one at kTextScale.
     */
    [[nodiscard]] antwika::ui::Theme editorTheme() noexcept;

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

        /** @brief How many voice lines the score is sounding. */
        std::size_t lines = 0;
    };

    /**
     * @brief The whole editor: one pane of code, what it says, and two
     * buttons.
     *
     * Stateless and deterministic, exactly as ui_demo::DemoScene is: the
     * same canvas, pointer, keyboard and state always describe the same
     * picture and answer the same way about what was pressed -- which is
     * what lets the entire layout be asserted with `EXPECT_EQ` and no
     * window, no mock and no backend.
     *
     * It holds none of the text, the caret or the focus.
     * Those arrive in the state and go back out through the frame,
     * because antwika::ui retains nothing between frames and neither may
     * this.
     *
     * **Its words are fixed English literals rather than an
     * i18n::Translator.**
     * The layout is a function of the words, and a session recorded in
     * one language and replayed in another would resolve a click to a
     * different widget.
     * ui_demo answers that by fixing the locale in `main()`; this app
     * answers it by having one language, which is the same guarantee
     * with nothing to configure -- and the words that matter here are
     * the score, which is not English in the first place.
     */
    class EditorScene final
    {
    public:
        /**
         * @brief Describe the editor as it now stands.
         * @param state The document, the caret and whether it is paused.
         * @param score What the document currently parses into.
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
