#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/Size.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"

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
    } // namespace widgets

    /**
     * @brief What each tool's palette button is labelled.
     * @param tool The tool to name.
     * @return The label, in BuildTool order.
     */
    [[nodiscard]] constexpr std::string_view toolLabel(
        BuildTool tool) noexcept
    {
        constexpr std::array<std::string_view, kBuildToolCount> labels{
            "road", "house", "food", "water", "fire", "arch"};

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
            widgets::toolWidget(BuildTool::FoodSource),
            widgets::toolWidget(BuildTool::WaterSource),
            widgets::toolWidget(BuildTool::FireStation),
            widgets::toolWidget(BuildTool::ArchitectPost)),
        "every toolbar widget needs its own id");

    /**
     * @brief What the pause button says, given what it would do.
     * @param paused Whether the simulation is being held still.
     * @return "resume" while paused, "pause" while it is running.
     *
     * Labelled with what pressing it does rather than with the state it
     * is in, so it reads as an instruction rather than as a status --
     * which is what the held-down appearance beside it is for.
     */
    [[nodiscard]] constexpr std::string_view pauseLabel(
        bool paused) noexcept
    {
        return paused ? "resume" : "pause";
    }

    /**
     * @brief The bar of buttons drawn over the grid.
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
         * @return The drawing commands and what the pointer did.
         *
         * The last three are defaulted so that a caller with nothing to
         * say about them -- a test whose subject is the zoom, or a
         * layout assertion -- writes only what it means.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Camera &camera,
            std::optional<BuildTool> selected = BuildTool::Road,
            bool paused = false,
            antwika::time::Tick tick = 0) const;
    };

} // namespace antwika::game
