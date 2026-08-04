#pragma once

#include <cstdint>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Bitmap;
    using antwika::gfx::Color;
    using antwika::gfx::Size;

    /**
     * @brief The image being edited, and the only thing a session
     * changes that anybody wants back.
     *
     * A plain value wrapping a gfx::Bitmap rather than a bitmap with
     * loose functions over it: what the pixels mean -- straight,
     * non-premultiplied RGBA, row by row, complete -- is an invariant,
     * and a class is where an invariant can be established once at the
     * door and relied on everywhere after it.
     *
     * Freely copyable, like every other value here: it owns a vector and
     * borrows nothing, so there is no lifetime rule to get wrong.
     *
     * It holds no undo history, deliberately. A stack of past states
     * would be the one piece of application state a replay could not
     * regenerate from the recorded clicks unless every entry of it were
     * regenerated too -- and every edit already *is* recorded, so
     * replaying a session up to a point is the undo this design has.
     */
    class Canvas final
    {
    public:
        /**
         * @brief Take an image somebody decoded.
         * @param image The pixels, which must be complete.
         * @param revision Where the change count starts, zero for a
         * freshly opened sheet; a state dump restores the count it
         * carried, so a renderer's upload key reads as it did.
         * @throws AtlasEditorError If the bitmap does not hold exactly
         * the pixels it claims to, since every later access indexes into
         * it arithmetically.
         */
        explicit Canvas(Bitmap image, std::uint64_t revision = 0);

        /**
         * @brief Make a fully transparent image to draw on.
         * @param size How big to make it.
         * @return The empty canvas.
         * @throws AtlasEditorError If either dimension is zero, which is
         * a sheet nothing could ever be painted on.
         */
        [[nodiscard]] static Canvas blank(Size size);

        /**
         * @brief Get how big the image is.
         * @return Its size in pixels.
         */
        [[nodiscard]] Size size() const noexcept;

        /**
         * @brief Read one pixel.
         * @param pixel Which pixel to read; it need not be inside the
         * image.
         * @return Its colour, or a fully transparent one when the pixel
         * lies outside the image -- which is what is beyond the edge of
         * a sheet as far as anything drawing it is concerned.
         */
        [[nodiscard]] Color at(Pixel pixel) const noexcept;

        /**
         * @brief Check whether a pixel is inside the image at all.
         * @param pixel The pixel to ask about.
         * @return True when both coordinates fall inside the size.
         */
        [[nodiscard]] bool holds(Pixel pixel) const noexcept;

        /**
         * @brief Write one pixel.
         *
         * Writing the colour a pixel already holds is not a change, so a
         * drag that crosses one pixel ten times leaves one edit behind
         * rather than ten -- and, more usefully, leaves the revision
         * where it was, so nothing re-uploads a texture that has not
         * changed.
         *
         * @param pixel Which pixel to write; one outside the image is
         * ignored rather than an error, since a drag off the edge of the
         * sheet is ordinary input.
         * @param color The colour to put there.
         * @return True when the image actually changed.
         */
        bool set(Pixel pixel, Color color) noexcept;

        /**
         * @brief Get the pixels, ready to upload or to encode.
         * @return The bitmap, always complete.
         */
        [[nodiscard]] const Bitmap &bitmap() const noexcept;

        /**
         * @brief Get how many times this image has changed.
         *
         * What a renderer holding a texture compares against to decide
         * whether to upload again: a count rather than a flag, because a
         * flag has to be cleared by whoever read it, and two readers
         * would then clear it from under each other.
         *
         * @return The number of changes since this canvas was made.
         */
        [[nodiscard]] std::uint64_t revision() const noexcept;

    private:
        Bitmap image;
        std::uint64_t changes = 0;
    };

} // namespace antwika::atlas_editor
