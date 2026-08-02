#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/WidgetId.hpp"

#include "FocusRing.hpp"
#include "Interactive.hpp"
#include "NodeKind.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief The index that means "no node".
     *
     * Every link in a Node holds either a real index or this.
     */
    inline constexpr std::size_t kNoNode =
        std::numeric_limits<std::size_t>::max();

    /**
     * @brief One entry in the layout arena.
     *
     * Deliberately one struct for both node kinds rather than a variant.
     * The two passes over the arena are flat index loops that touch every
     * entry, and a handful of unused fields on a text node costs less
     * than a visit at every step of them.
     *
     * The four link fields are what make the arena a tree.
     * They are only ever written by LayoutTree, which is what guarantees
     * the invariant both passes rely on: a child's index is always
     * greater than its parent's.
     */
    struct Node
    {
        NodeKind kind = NodeKind::Container;

        /**
         * @brief Which way this container stacks its children.
         */
        Axis axis = Axis::Column;

        /**
         * @brief How this node's own width is decided.
         */
        Sizing width{};

        /**
         * @brief How this node's own height is decided.
         */
        Sizing height{};

        /**
         * @brief Where this container puts its children across its axis.
         */
        Alignment cross = Alignment::Start;

        /**
         * @brief Pixels of inset on every side of this container.
         */
        std::uint32_t padding = 0;

        /**
         * @brief Pixels between one child of this container and the next.
         */
        std::uint32_t gap = 0;

        /**
         * @brief The colour to fill this node's area with, if any.
         *
         * An optional rather than a fully transparent colour, because
         * antwika::gfx promises nothing about blending, so treating a
         * zero alpha as "draw nothing" would be reading a guarantee that
         * was never given.
         */
        std::optional<Color> background{};

        /**
         * @brief Which widget a pointer landing here is landing on.
         *
         * kNoWidget for everything the caller did not name, which is
         * what keeps a button's own spacers and label from being hit
         * instead of the button.
         */
        WidgetId id = kNoWidget;

        /**
         * @brief The colours this node's background is resolved from.
         *
         * Absent for every node whose background is already decided:
         * anything that is not interactive, and a button whose caller
         * said how it must look.
         */
        std::optional<Interactive> style{};

        /**
         * @brief The border this node would draw if it were focused.
         *
         * Present exactly on the nodes the keyboard can reach, so this
         * is what makes a node focusable as well as what says how the
         * ring looks. Absent on everything else, including a container
         * and a button's own label.
         *
         * A node carrying one but named kNoWidget is skipped all the
         * same: focus crosses back into application state as an id, so
         * a widget nothing can name is a widget nothing can focus.
         */
        std::optional<FocusRing> focusStyle{};

        /**
         * @brief The border this node is actually drawing, if any.
         *
         * focusStyle is the source and this is the resolved answer,
         * exactly as style is the source of background. Written by
         * resolve() on the focused node alone.
         */
        std::optional<FocusRing> focusRing{};

        /**
         * @brief Whether this node is drawing its pressed appearance.
         *
         * A third resolved answer beside background and focusRing, and
         * written by resolve() for the same reason they are: only there
         * are the pointer and the layout both known.
         *
         * It exists so flatten() can tell a widget the recorded pointer
         * is holding down from one merely sitting idle, without being
         * handed the pointer to work it out again. A hover pass leaves
         * the held one alone, since a press is recorded input and its
         * appearance is the simulation's answer rather than a hint's.
         */
        bool pressed = false;

        /**
         * @brief Whether children past this container's edge are cut
         * off rather than everything being squeezed to fit.
         *
         * False everywhere but a text area's lines, and that is the
         * whole reason it exists: a container with more asked of it
         * than it has room for cuts every child down in proportion,
         * which for a document longer than its pane is a page of lines
         * too short to draw a glyph in -- a blank pane.
         *
         * Set here instead, the children keep the size they asked for
         * and the ones past the bottom edge are clamped to nothing by
         * the placement that already keeps a child inside its parent.
         * Which is what a scroll wants: whole lines, and no half ones.
         */
        bool clips = false;

        /**
         * @brief The characters this text node draws.
         */
        std::string text{};

        /**
         * @brief Pixels per glyph pixel for this text node.
         */
        std::uint32_t textScale = 0;

        /**
         * @brief The colour this text node draws in.
         */
        Color textColor{};

        /**
         * @brief Whether this node belongs to an overlay.
         *
         * True for every node of an open dropdown's list, and what
         * flatten() and resolve() partition on: an overlay is painted
         * after everything else, so it is on top, and hit-tested before
         * everything else, so it is on top there too.
         */
        bool overlay = false;

        /**
         * @brief The node this overlay hangs beneath, if it is one.
         *
         * Set on an overlay's root alone, and what takes that root out
         * of its parent's flow: an overlay occupies no room where it was
         * declared, and is placed against this node once that node has
         * been arranged.
         */
        std::size_t overlayAnchor = kNoNode;

        /**
         * @brief Which dropdown this node is an option of, if any.
         */
        WidgetId optionOwner = kNoWidget;

        /**
         * @brief Which of that dropdown's options this node is.
         */
        std::size_t optionIndex = 0;

        std::size_t parent = kNoNode;
        std::size_t firstChild = kNoNode;
        std::size_t lastChild = kNoNode;
        std::size_t nextSibling = kNoNode;

        /**
         * @brief What this node's content needs, filled in by measuring.
         */
        Size measured{};

        /**
         * @brief Where this node ended up, filled in by arranging.
         */
        Rect arranged{};
    };

} // namespace antwika::ui::detail
