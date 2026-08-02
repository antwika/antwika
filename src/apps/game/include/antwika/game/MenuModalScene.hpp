#pragma once

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the menu modal's items are called.
     *
     * Its own namespace rather than the toolbar's `widgets`, for the
     * reason `menuWidgets` has one: the two are resolved against
     * different frames and never share one, so a number colliding across
     * them would be a merge hazard and never a bug the app could see.
     *
     * None of these ever reaches a replay: what is recorded is the click,
     * and which item it hit is worked out again from it -- see UiSink.
     */
    namespace modalWidgets
    {
        /**
         * @brief Leave the city for the main menu.
         */
        inline constexpr WidgetId kMainMenu{201};

        /**
         * @brief Put the modal away and carry on where it was opened.
         */
        inline constexpr WidgetId kResume{202};
    } // namespace modalWidgets

    /**
     * @brief The menu drawn over a city, opened by the bar's menu button.
     *
     * **This is a modal rather than a mode**, which is the one thing
     * that separates it from MainMenuScene: the city is still there
     * behind it, still holding its camera, its palette and its
     * buildings, and closing the modal puts a player back exactly where
     * they were. AppMode.hpp says a mode is a whole application state
     * and that a modal stack would be the wrong shape there, and this
     * does not contradict that: leaving for the main menu is still a
     * mode change, asked for on AppModeState like every other.
     *
     * The scrim is a container the size of the whole canvas with a fill
     * behind it, and that is load-bearing rather than decoration: it is
     * what makes ui::Interactions::pointerOverUi true wherever the
     * pointer is, so GridSink's existing "what the UI covers, it covers
     * from the grid too" rule keeps every press away from the city with
     * no second mechanism invented for it -- see UiOverlay.
     *
     * Stateless and deterministic, like MainMenuScene and Toolbar: the
     * same canvas and pointer always produce the same picture and the
     * same answer about what was pressed, which is what lets a whole
     * modal be asserted with EXPECT_EQ and no mock.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, for the reason
     * Toolbar gives: a hit-test is a function of the layout and the
     * layout is a function of the canvas.
     *
     * There is no draw() here, unlike MainMenuScene's. A mode owns the
     * whole screen and so clears it; this is drawn over a city that has
     * already been painted, so its commands are appended to the
     * toolbar's in the one UiOverlay RenderSystem paints last.
     */
    class MenuModalScene final
    {
    public:
        /**
         * @brief Construct the modal over the language it words itself
         * in.
         *
         * Injected and fixed at kDefaultLocale by whoever builds it, for
         * the reason Toolbar gives.
         *
         * @param translator Words every caption; must outlive this
         * scene.
         */
        explicit MenuModalScene(const Translator &translator);

        /**
         * @brief Describe the modal for one tick.
         * @param canvas The area the modal is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(Size canvas, Pointer pointer) const;

    private:
        const Translator &translator;
    };

} // namespace antwika::game
