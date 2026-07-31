#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include <antwika/gfx/Size.hpp>
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
         * @brief The palette's first button, one per BuildTool.
         *
         * The tools run from here in their declaration order, so a tool
         * and its button cannot drift apart -- toolWidget() is the one
         * place that mapping is written.
         */
        inline constexpr WidgetId kFirstTool{4};

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
            widgets::toolWidget(BuildTool::Road),
            widgets::toolWidget(BuildTool::House),
            widgets::toolWidget(BuildTool::FoodSource),
            widgets::toolWidget(BuildTool::WaterSource),
            widgets::toolWidget(BuildTool::FireStation),
            widgets::toolWidget(BuildTool::ArchitectPost)),
        "every toolbar widget needs its own id");

    /**
     * @brief The bar of buttons drawn over the grid.
     *
     * A pure function of the canvas, the pointer and the camera, so the
     * same three always produce the same picture and the same answer
     * about what the pointer is on.
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
         * to know.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const Camera &camera,
            BuildTool selected = BuildTool::Road) const;
    };

} // namespace antwika::game
