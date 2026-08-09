#pragma once

#include <vector>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Tool.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    /**
     * @brief Walks the pixels a straight line covers.
     *
     * @param from The pixel the line starts on.
     * @param to The pixel the line ends on.
     * @param within The sheet, outside which pixels are dropped.
     * @return The pixels in order, from first to last.
     */
    [[nodiscard]] std::vector<Pixel> linePixels(
        Pixel from, Pixel to, Size within);

    /**
     * @brief Walks the outline of the ellipse a drag boxes in.
     *
     * @param from The pixel one corner of the box sits on.
     * @param to The pixel the opposite corner sits on.
     * @param within The sheet, outside which pixels are dropped.
     * @return The outline pixels, in reading order.
     */
    [[nodiscard]] std::vector<Pixel> ellipsePixels(
        Pixel from, Pixel to, Size within);

    /**
     * @brief Walks the pixels a shape tool lays down between two pixels.
     *
     * @param tool The tool drawing, an ellipse for Tool::Ellipse and a
     *             line for any other.
     * @param from The pixel the drag began on.
     * @param to The pixel the drag reached.
     * @param within The sheet, outside which pixels are dropped.
     * @return The pixels the shape covers.
     */
    [[nodiscard]] std::vector<Pixel> shapePixels(
        Tool tool, Pixel from, Pixel to, Size within);

}
