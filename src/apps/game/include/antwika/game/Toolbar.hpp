#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/MenuItem.hpp"
#include "antwika/game/MessageId.hpp"
#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the toolbar's buttons are called.
     *
     * Symbolic names rather than where a button ended up in the layout,
     * because this is what crosses back into the simulation. None of
     * these ever reaches a replay: what is recorded is the click, and
     * which button it hit is worked out again from it -- see UiSink.
     */
    namespace widgets
    {
        /**
         * @brief Zoom one level out.
         *
         * The view controls and the readouts sit on the bottom bar and
         * the palette down the right, so the numbers below say nothing
         * about where a button is -- which is the whole point of an id
         * being symbolic rather than positional.
         */
        inline constexpr WidgetId kZoomOut{1};

        /**
         * @brief Zoom one level in.
         */
        inline constexpr WidgetId kZoomIn{2};

        /**
         * @brief Put the camera back where the run started.
         */
        inline constexpr WidgetId kResetView{3};

        /**
         * @brief Hold the simulation still, or let it go again.
         *
         * One button rather than two, because what it does is toggle a
         * single fact: a "pause" and a "resume" would need one of them
         * disabled at all times, which is two widgets saying one thing.
         */
        inline constexpr WidgetId kPauseResume{4};

        /**
         * @brief The palette's first button, one per BuildTool.
         *
         * The tools run from here in their declaration order, so a tool
         * and its button cannot drift apart -- toolWidget() is the one
         * place that mapping is written.
         */
        inline constexpr WidgetId kFirstTool{5};

        /**
         * @brief Open the menu modal over the city.
         *
         * Numbered past the palette rather than before it, since the
         * tools run from kFirstTool in their own order and a number
         * inserted among them would rename every button after it.
         */
        inline constexpr WidgetId kMenu{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool) + kBuildToolCount)};

        /**
         * @brief The closed box of the top bar's game menu.
         *
         * Derived from kMenu rather than written out, for the reason
         * kMenu is derived from kFirstTool: a tool added to the palette
         * moves every number after it, and deriving is what keeps them
         * from colliding when it does.
         */
        inline constexpr WidgetId kGameMenu{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kMenu) + 1)};

        /**
         * @brief The game menu's first item, one per MenuItem.
         *
         * antwika::ui names option `n` of a list kFirstMenuItem plus
         * `n`, so the whole run of kMenuItemCount ids is spoken for.
         */
        inline constexpr WidgetId kFirstMenuItem{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kMenu) + 2)};

        /**
         * @brief The strip along the top, holding the game menu.
         *
         * Named so its area is reported in ui::Frame::rects, which is
         * how anything asking what the bars leave for the city asks the
         * layout rather than working it out a second time.
         */
        inline constexpr WidgetId kTopBar{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstMenuItem)
                + kMenuItemCount)};

        /**
         * @brief The panel down the right, holding the build palette.
         */
        inline constexpr WidgetId kSidePanel{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kTopBar) + 1)};

        /**
         * @brief The strip along the bottom, holding the view controls
         *        and the readouts.
         */
        inline constexpr WidgetId kBottomBar{
            static_cast<WidgetId>(
                static_cast<std::uint64_t>(kTopBar) + 2)};

        /**
         * @brief Get which button selects a tool.
         * @param tool The tool to ask about.
         * @return That tool's palette button.
         */
        [[nodiscard]] constexpr WidgetId toolWidget(BuildTool tool) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstTool)
                + buildToolIndex(tool));
        }

        /**
         * @brief Get which option of the game menu an item is.
         * @param item The item to ask about.
         * @return That item's option in the dropped-down list.
         */
        [[nodiscard]] constexpr WidgetId menuItemWidget(
            MenuItem item) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstMenuItem)
                + menuItemIndex(item));
        }
    } // namespace widgets

    /**
     * @brief What each tool's palette button is labelled.
     *
     * **A MessageId rather than the English word.** A label is read by a
     * person, so it goes through antwika::i18n like every other caption;
     * the id is what the palette knows and the words are the
     * translator's. The id is never persisted, so its numbering is free
     * -- see MessageId.hpp.
     *
     * @param tool The tool to name.
     * @return The label's id, in BuildTool order.
     */
    [[nodiscard]] constexpr MessageId toolLabel(BuildTool tool) noexcept
    {
        constexpr std::array<MessageId, kBuildToolCount> labels{
            MessageId::ToolRoad,
            MessageId::ToolHouse,
            MessageId::ToolFarm,
            MessageId::ToolClayPit,
            MessageId::ToolWorkshop,
            MessageId::ToolStorage,
            MessageId::ToolMarket,
            MessageId::ToolWell,
            MessageId::ToolDoctor,
            MessageId::ToolFireStation,
            MessageId::ToolEngineerPost};

        return labels[buildToolIndex(tool) % kBuildToolCount];
    }

    // Two widgets sharing an id would be one widget, silently.
    // The palette's ids are derived, so this is where that is checked.
    static_assert(
        antwika::ui::assertDistinct(
            widgets::kZoomOut,
            widgets::kZoomIn,
            widgets::kResetView,
            widgets::kPauseResume,
            widgets::toolWidget(BuildTool::Road),
            widgets::toolWidget(BuildTool::House),
            widgets::toolWidget(BuildTool::Farm),
            widgets::toolWidget(BuildTool::ClayPit),
            widgets::toolWidget(BuildTool::Workshop),
            widgets::toolWidget(BuildTool::Storage),
            widgets::toolWidget(BuildTool::Market),
            widgets::toolWidget(BuildTool::Well),
            widgets::toolWidget(BuildTool::Doctor),
            widgets::toolWidget(BuildTool::FireStation),
            widgets::toolWidget(BuildTool::EngineerPost),
            widgets::kMenu,
            widgets::kGameMenu,
            widgets::menuItemWidget(MenuItem::NewGame),
            widgets::menuItemWidget(MenuItem::SaveGame),
            widgets::menuItemWidget(MenuItem::LoadGame),
            widgets::menuItemWidget(MenuItem::MainMenu),
            widgets::menuItemWidget(MenuItem::WorldMap),
            widgets::kTopBar,
            widgets::kSidePanel,
            widgets::kBottomBar),
        "every toolbar widget needs its own id");

    // Two tools sharing a caption would be two buttons reading the same.
    // The table above is where that can happen, so it is checked here.
    static_assert(
        []
        {
            for (std::size_t left = 0; left < kBuildToolCount; ++left)
            {
                for (std::size_t right = left + 1; right < kBuildToolCount;
                     ++right)
                {
                    if (toolLabel(static_cast<BuildTool>(left))
                        == toolLabel(static_cast<BuildTool>(right)))
                    {
                        return false;
                    }
                }
            }

            return true;
        }(),
        "every tool needs a caption of its own");

    /**
     * @brief What the pause button says, given what it would do.
     * @param paused Whether the simulation is being held still.
     * @return The "resume" id while paused, the "pause" one while it is
     * running.
     *
     * Labelled with what pressing it does rather than with the state it
     * is in, so it reads as an instruction rather than as a status --
     * which is what the held-down appearance beside it is for.
     */
    [[nodiscard]] constexpr MessageId pauseLabel(bool paused) noexcept
    {
        return paused ? MessageId::ToolbarResume
                      : MessageId::ToolbarPause;
    }

    /**
     * @brief The furniture drawn round the grid.
     *
     * Three pieces against one canvas: a strip along the top carrying
     * the game menu and the way into the menu modal, a panel down the
     * right carrying the build palette, and a strip along the bottom
     * carrying the view controls and every readout. What is left in the
     * middle is the city, and nothing is drawn over it -- the gap is a
     * spacer, which fills no pixels and so covers none, which is what
     * keeps a click there the grid's.
     *
     * A pure function of the canvas, the pointer and the simulation
     * state it reports, so the same arguments always produce the same
     * picture and the same answer about what the pointer is on.
     *
     * **Every number it shows is simulation state**: the zoom, the
     * selected tool, whether the run is paused, and which tick it is on.
     * A replay regenerates all four, so the bar a replay draws is the
     * bar the live run drew. The frame rate is deliberately not here --
     * it comes off a wall clock, so it is described on the render side
     * instead, by describeFps().
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, because a hit-test
     * is a function of the layout and the layout is a function of the
     * canvas: resolving a recorded click against a differently sized
     * window would resolve it to a different button.
     */
    class Toolbar final
    {
    public:
        /**
         * @brief Construct the bar over the language it words itself in.
         *
         * **Injected, and fixed at kDefaultLocale by whoever builds
         * it.** A layout is a function of the strings declared into it
         * and a hit-test is a function of the layout, so a bar worded in
         * one language and replayed in another would resolve the same
         * recorded click to a different button -- see Translator.hpp.
         *
         * @param translator Words every caption; must outlive this bar.
         */
        explicit Toolbar(const Translator &translator);

        /**
         * @brief Describe the toolbar for one tick.
         * @param canvas The area the UI is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param camera The camera whose zoom the bar reports.
         * @param selected The tool whose palette button is shown as
         * chosen; the appearance is forced rather than worked out from
         * the pointer, since which tool is selected is the application's
         * to know. Nullopt is the palette put down, and then no button
         * is held: a bar showing a tool nobody has selected would say
         * a left click places something when it places nothing.
         * @param paused Whether the simulation is being held still,
         * which is what the pause button is labelled from.
         * @param tick The tick the bar reports, which is the tick this
         * describe() is part of.
         * @param ratings How the city is doing, reported as the last two
         * readouts on the bottom bar. A pure function of the World, so a
         * replay regenerates it exactly as it regenerates the zoom --
         * see CityRatings.
         * @param menuOpen Whether the game menu's list is dropped down.
         * **Simulation state, in the camera's sense**: it decides what a
         * click at a pixel means, so it is passed in rather than kept
         * here, is written inside the tick path by UiSink, and is never
         * persisted -- antwika::ui retains nothing of the kind either.
         * @return The drawing commands and what the pointer did.
         *
         * The last five are defaulted so that a caller with nothing to
         * say about them -- a test whose subject is the zoom, or a
         * layout assertion -- writes only what it means.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Camera &camera,
            std::optional<BuildTool> selected = BuildTool::Road,
            bool paused = false,
            antwika::time::Tick tick = 0,
            CityRatings ratings = {},
            bool menuOpen = false) const;

    private:
        const Translator &translator;
    };

} // namespace antwika::game
