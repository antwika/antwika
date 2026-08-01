#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawList.hpp>

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::DrawList;

    /**
     * @brief What the readout shows while there is no rate to show yet.
     *
     * Two dashes rather than a number, because a run that has been
     * drawing for less than a second has measured nothing, and "0" is a
     * measurement -- one that says the machine is drawing no frames at
     * all, which is the opposite of what is happening. It is the same
     * width in this font whatever the eventual number is, so the corner
     * does not jump when the first window closes.
     */
    inline constexpr std::string_view kNoRateYet{"--"};

    /**
     * @brief Describe the frame-rate readout drawn in the top corner.
     *
     * **Described on the render side, unlike every other picture in this
     * app.** The toolbar, the menu and the save screen are all described
     * by a sink inside the tick path, because a click has to be resolved
     * against the same layout that is drawn. Nothing here is clickable,
     * and the number it shows comes off a wall clock -- so describing it
     * in the tick path would put the machine's speed where a replay
     * could see it. It therefore has no widget id, reports no
     * interactions and never reaches UiOverlay.
     *
     * A DrawList rather than a ui::Frame for the same reason: there are
     * no interactions to hand back and no widget rectangles to place
     * anything against.
     *
     * @param canvas The area the readout is laid out into.
     * @param framesPerSecond The rate to show, straight from
     * FrameMeter::perSecond(); nothing at all shows kNoRateYet in its
     * place, which is what the first second of a run draws.
     * @return The drawing commands, in the order they draw.
     */
    [[nodiscard]] DrawList describeFps(
        Size canvas, std::optional<std::uint32_t> framesPerSecond);

} // namespace antwika::game
