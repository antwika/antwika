#pragma once

#include <optional>
#include <vector>

#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/WidgetId.hpp"

namespace antwika::ui
{

    using antwika::gfx::Rect;

    /**
     * @brief Where one named node of one frame was laid out.
     */
    struct WidgetRect
    {
        /**
         * @brief The id the caller named that node with.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief The area the layout arranged that node into.
         *
         * The very rectangle the picture was drawn from, read out of the
         * arranging pass rather than worked out again beside it.
         */
        Rect rect{};

        /**
         * @brief Compare two entries.
         * @param other The entry to compare against.
         * @return True when the id and the rectangle both match.
         */
        [[nodiscard]] bool operator==(const WidgetRect &other) const =
            default;
    };

    /**
     * @brief Where every named node of one frame was laid out.
     *
     * The answer to the one question a DrawList cannot be asked: not
     * "what was drawn" but "where did the thing I named end up".
     * An application drawing its own art around a UI -- a card behind a
     * label, a portrait inside a seat -- has to place that art somewhere,
     * and the only place that cannot drift is the layout this library
     * already produced.
     * Computing it a second time in application code is the bug this
     * exists to make unnecessary: two independent layouts agree until the
     * moment either one changes.
     *
     * **Every named node reports, not only containers.** A node carries
     * one id, whatever kind it is, and a button's rectangle is as useful
     * to something drawing behind it as a row's is.
     * Restricting this to containers would mean a second rule about which
     * ids answer, and an application would have to wrap a button in a
     * named row to find out where the button went.
     *
     * **One entry per distinct id, and the last declaration wins.** Two
     * nodes sharing an id is already legal and already means they are one
     * widget, so this reports one rectangle for that widget rather than
     * two: the one belonging to the node declared last, which is the one
     * painted over the other.
     * Use assertDistinct() to turn an unintended repeat into a build
     * error, exactly as for hovering and activation.
     *
     * **Nothing is retained between frames**, as everywhere else here.
     * This is a value handed back with the picture it belongs to, and it
     * describes that frame alone.
     *
     * Reading it back is safe anywhere, including inside the tick path a
     * replay reproduces: a layout is a pure function of the declarations,
     * the theme and the canvas, so a replay lays out exactly what the
     * recorded run did.
     * That is what makes it unlike input::PointerHintChannel, which a
     * replay deliberately cannot reproduce.
     * The existing rule about the canvas still holds, and is the whole of
     * the condition: lay out against the configured window size, never
     * the size a window reports.
     *
     * The entries are a flat vector in declaration order rather than a
     * map, because a frame names a handful of widgets and a linear scan
     * over a handful beats hashing them.
     * It is also what makes this free for a caller that names nothing:
     * an empty vector allocates nothing, so a display-only frame pays one
     * integer comparison per node in a loop that already runs and not one
     * byte of storage.
     */
    struct WidgetRects
    {
        /**
         * @brief One entry per distinct id, in declaration order.
         */
        std::vector<WidgetRect> entries{};

        /**
         * @brief Find where one widget was laid out.
         *
         * Absent rather than an empty rectangle when the id is not here,
         * since a collapsed widget is a real answer a caller may want to
         * tell apart from a widget this frame never declared.
         * A frame declares itself afresh, so an absent id means exactly
         * that this frame did not declare it -- draw nothing for it, and
         * do not fall back to a rectangle of your own.
         *
         * kNoWidget is never an entry, so asking for it always answers
         * nothing.
         *
         * @param id The widget to look for.
         * @return Its arranged area, or nothing when this frame declared
         * no node carrying that id.
         */
        [[nodiscard]] std::optional<Rect> find(WidgetId id) const;

        /**
         * @brief Compare two frames' mappings.
         * @param other The mapping to compare against.
         * @return True when both hold the same entries in the same order.
         */
        [[nodiscard]] bool operator==(const WidgetRects &other) const =
            default;
    };

} // namespace antwika::ui
