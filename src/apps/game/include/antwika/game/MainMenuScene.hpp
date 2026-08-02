#pragma once

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

namespace antwika::game
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;
    using antwika::ui::DrawList;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the main menu's items are called.
     *
     * Its own namespace rather than the toolbar's `widgets`, because the
     * two are resolved in different modes and never share a frame; a
     * number colliding across them would be a merge hazard and never a
     * bug the app could see.
     *
     * None of these ever reaches a replay: what is recorded is the click,
     * and which item it hit is worked out again from it -- see
     * MainMenuSink.
     */
    namespace menuWidgets
    {
        /**
         * @brief Leave the menu for an empty grid.
         */
        inline constexpr WidgetId kNewGame{101};

        /**
         * @brief End the run.
         */
        inline constexpr WidgetId kQuit{102};

        /**
         * @brief Open the picker a session is read back from.
         */
        inline constexpr WidgetId kLoadGame{103};

        /**
         * @brief Leave the menu for the world and its cities.
         */
        inline constexpr WidgetId kWorldMap{104};
    } // namespace menuWidgets

    /**
     * @brief The main menu: the whole screen, in the mode of its own name.
     *
     * Not a modal. Nothing of the grid is drawn behind it, because
     * nothing of the grid is running behind it -- see AppMode.hpp.
     *
     * Stateless and deterministic, like GridScene and Toolbar: the same
     * canvas and pointer always produce the same picture and the same
     * answer about what was pressed, which is what lets a whole menu be
     * asserted with EXPECT_EQ and no mock.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, for the reason
     * Toolbar gives: a hit-test is a function of the layout, and the
     * layout is a function of the canvas.
     */
    class MainMenuScene final
    {
    public:
        /**
         * @brief Construct the menu over the language it words itself
         * in.
         *
         * Injected and fixed at kDefaultLocale by whoever builds it, for
         * the reason Toolbar gives: a hit-test is a function of a layout
         * laid out from translated text.
         *
         * @param translator Words every caption; must outlive this
         * scene.
         */
        explicit MainMenuScene(const Translator &translator);

        /**
         * @brief Describe the menu for one tick.
         * @param canvas The area the menu is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(Size canvas, Pointer pointer) const;

        /**
         * @brief Draw one frame of the menu.
         *
         * Clears first, because a mode owns the whole screen: there is
         * nothing underneath the menu for it to be drawn over.
         *
         * @param renderer Receives the drawing calls.
         * @param picture The commands describe() produced, by way of the
         * overlay MainMenuSink wrote them into.
         */
        void draw(IRenderer &renderer, const DrawList &picture) const;

    private:
        const Translator &translator;
    };

} // namespace antwika::game
