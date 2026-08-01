#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/SaveLoadState.hpp"

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the save/load screen's widgets are called.
     *
     * Its own namespace rather than the menu's or the toolbar's, because
     * the three are resolved in different modes and never share a frame.
     *
     * None of these ever reaches a replay: what is recorded is the click
     * or the keystroke, and which widget it hit is worked out again from
     * it -- see SaveLoadSink.
     */
    namespace saveWidgets
    {
        /**
         * @brief The box listing the saves that exist.
         */
        inline constexpr WidgetId kPicker{201};

        /**
         * @brief The field a new save's name is typed into.
         */
        inline constexpr WidgetId kName{202};

        /**
         * @brief Write the session out.
         */
        inline constexpr WidgetId kSave{203};

        /**
         * @brief Read the selected save back into the session.
         */
        inline constexpr WidgetId kLoad{204};

        /**
         * @brief Go back to the main menu.
         */
        inline constexpr WidgetId kBack{205};

        /**
         * @brief The picker's first option; option n carries this plus n.
         *
         * Far above the five widgets beside it, so a directory holding
         * ninety-odd saves still cannot collide with one of them.
         */
        inline constexpr WidgetId kFirstOption{300};
    } // namespace saveWidgets

    /**
     * @brief The screen a session is written to or read back from.
     *
     * Stateless and deterministic, like MainMenuScene and Toolbar: the
     * same canvas, pointer, keyboard and SaveLoadState always produce
     * the same picture and the same answer about what was pressed --
     * which is what lets the whole screen be asserted with EXPECT_EQ and
     * no mock.
     *
     * It holds nothing of the field's characters, the caret, the list's
     * open flag or the focus. All of those arrive in the state and go
     * back out through the frame, because antwika::ui retains nothing
     * between frames and neither may this.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, for the reason
     * Toolbar gives: a hit-test is a function of the layout, and the
     * layout is a function of the canvas.
     */
    class SaveLoadScene final
    {
    public:
        /**
         * @brief Construct the screen over the language it words itself
         * in.
         *
         * Injected and fixed at kDefaultLocale by whoever builds it, for
         * the reason Toolbar gives.
         *
         * @param translator Words every caption; must outlive this
         * scene.
         */
        explicit SaveLoadScene(const Translator &translator);

        /**
         * @brief Describe the screen for one tick.
         * @param canvas The area the screen is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param keyboard The key edges and characters this tick.
         * @param state The field, the list, the focus and the message.
         * @return The drawing commands and what the input did.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Keyboard &keyboard,
            const SaveLoadState &state) const;

        /**
         * @brief Draw one frame of the screen.
         *
         * Clears first, because a mode owns the whole screen: there is
         * nothing underneath this for it to be drawn over.
         *
         * @param renderer Receives the drawing calls.
         * @param picture The commands describe() produced, by way of the
         * overlay SaveLoadSink wrote them into.
         */
        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

} // namespace antwika::game
