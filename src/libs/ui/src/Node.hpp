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
