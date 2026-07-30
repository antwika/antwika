#include "Layout.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/TextLayout.hpp>

#include "antwika/ui/Alignment.hpp"
#include "antwika/ui/Axis.hpp"
#include "antwika/ui/Sizing.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"
#include "Saturate.hpp"

namespace antwika::ui::detail
{

    using antwika::gfx::Point;
    using antwika::gfx::Rect;

    namespace
    {
        /**
         * @brief Where a container's children go, and how.
         *
         * One value rather than six parameters, since placement needs
         * all of it and none of it separately.
         */
        struct ContentBox
        {
            Point origin;
            Size size;
            Axis axis;
            Alignment cross;
            std::uint32_t gap;
        };

        std::uint32_t saturatingSub(
            std::uint32_t value, std::uint32_t amount) noexcept
        {
            return value > amount ? value - amount : 0;
        }

        std::uint32_t doubled(std::uint32_t value) noexcept
        {
            return clampToU32(std::uint64_t{value} * 2);
        }

        std::uint32_t mainOf(Size size, Axis axis) noexcept
        {
            return axis == Axis::Row ? size.width : size.height;
        }

        std::uint32_t crossOf(Size size, Axis axis) noexcept
        {
            return axis == Axis::Row ? size.height : size.width;
        }

        std::int32_t mainOf(Point point, Axis axis) noexcept
        {
            return axis == Axis::Row ? point.x : point.y;
        }

        std::int32_t crossOf(Point point, Axis axis) noexcept
        {
            return axis == Axis::Row ? point.y : point.x;
        }

        Size sizeFrom(
            Axis axis, std::uint32_t main, std::uint32_t cross) noexcept
        {
            return axis == Axis::Row
                       ? Size{.width = main, .height = cross}
                       : Size{.width = cross, .height = main};
        }

        Point pointFrom(
            Axis axis, std::int32_t main, std::int32_t cross) noexcept
        {
            return axis == Axis::Row ? Point{.x = main, .y = cross}
                                     : Point{.x = cross, .y = main};
        }

        Sizing mainSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.width : node.height;
        }

        Sizing crossSizing(const Node &node, Axis axis) noexcept
        {
            return axis == Axis::Row ? node.height : node.width;
        }

        /**
         * @brief What a parent must reserve for a child along its axis.
         *
         * Fit and Grow answer the same here on purpose: a growing child
         * still asks for its content, so a container fitting around one
         * does not collapse.
         */
        std::uint32_t mainDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = mainSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                       ? sizing.pixels
                       : mainOf(node.measured, axis);
        }

        std::uint32_t crossDemand(const Node &node, Axis axis) noexcept
        {
            const auto sizing = crossSizing(node, axis);

            return sizing.mode == SizeMode::Fixed
                       ? sizing.pixels
                       : crossOf(node.measured, axis);
        }

        void measure(LayoutTree &tree)
        {
            for (auto index = tree.size(); index-- > 0;)
            {
                auto &node = tree.node(index);

                if (node.kind == NodeKind::Text)
                {
                    node.measured =
                        antwika::gfx::textSize(node.text, node.textScale);

                    continue;
                }

                std::uint64_t along = 0;
                std::uint32_t across = 0;
                std::uint32_t count = 0;

                for (auto child = node.firstChild; child != kNoNode;
                     child = tree.node(child).nextSibling)
                {
                    const auto &value = tree.node(child);

                    along += mainDemand(value, node.axis);
                    across =
                        std::max(across, crossDemand(value, node.axis));
                    ++count;
                }

                const auto inset = doubled(node.padding);
                const auto gaps =
                    count > 0 ? clampToU32(
                                    std::uint64_t{node.gap} * (count - 1))
                              : 0U;

                node.measured = sizeFrom(
                    node.axis,
                    clampToU32(along + gaps + inset),
                    clampToU32(std::uint64_t{across} + inset));
            }
        }

        /**
         * @brief Hand leftover room to the children that asked to grow.
         *
         * Growers share it equally, and the earliest of them take the
         * pixels the division could not split.
         */
        void distributeGrowth(
            const LayoutTree &tree,
            const std::vector<std::size_t> &children,
            std::vector<std::uint32_t> &extents,
            Axis axis,
            std::uint32_t slack,
            std::uint32_t growers)
        {
            const auto share = slack / growers;
            auto extra = slack % growers;

            for (std::size_t index = 0; index < children.size(); ++index)
            {
                const auto &value = tree.node(children[index]);

                if (mainSizing(value, axis).mode != SizeMode::Grow)
                {
                    continue;
                }

                std::uint32_t bonus = 0;

                if (extra > 0)
                {
                    bonus = 1;
                    --extra;
                }

                extents[index] = clampToU32(
                    std::uint64_t{extents[index]} + share + bonus);
            }
        }

        /**
         * @brief Cut every child in proportion to what it asked for.
         *
         * In proportion rather than first-come-first-served, so that a
         * container with too little room still shows every child instead
         * of giving the last ones nothing.
         */
        void distributeShrink(
            std::vector<std::uint32_t> &extents,
            std::uint64_t demand,
            std::uint32_t room)
        {
            std::uint64_t assigned = 0;

            for (auto &extent : extents)
            {
                // Only reached when demand exceeds room.
                // So demand is above zero and the division is safe.
                extent = static_cast<std::uint32_t>(
                    std::uint64_t{extent} * room / demand);

                assigned += extent;
            }

            // Each extent was rounded down, so this is under the count.
            // These are therefore the earliest children.
            const auto leftover = room - assigned;

            for (std::uint64_t index = 0; index < leftover; ++index)
            {
                ++extents[static_cast<std::size_t>(index)];
            }
        }

        void place(
            LayoutTree &tree,
            const std::vector<std::size_t> &children,
            const std::vector<std::uint32_t> &extents,
            const ContentBox &box)
        {
            const auto available = crossOf(box.size, box.axis);
            std::uint32_t cursor = 0;

            for (std::size_t index = 0; index < children.size(); ++index)
            {
                auto &value = tree.node(children[index]);
                const auto sizing = crossSizing(value, box.axis);

                auto extent =
                    sizing.mode == SizeMode::Fixed ? sizing.pixels
                    : sizing.mode == SizeMode::Grow
                        ? available
                        : crossOf(value.measured, box.axis);

                // This clamp is what keeps a child inside its parent.
                // It also makes the offsets below provably non-negative.
                extent = std::min(extent, available);

                std::uint32_t offset = 0;

                // Start is the fall-through rather than a third arm.
                // A switch would carry a fourth, out-of-range arm.
                // Only an invalid Alignment could ever reach that one.
                if (box.cross == Alignment::Center)
                {
                    offset = (available - extent) / 2;
                }
                else if (box.cross == Alignment::End)
                {
                    offset = available - extent;
                }

                value.arranged = Rect{
                    .origin = pointFrom(
                        box.axis,
                        mainOf(box.origin, box.axis)
                            + static_cast<std::int32_t>(cursor),
                        crossOf(box.origin, box.axis)
                            + static_cast<std::int32_t>(offset)),
                    .size =
                        sizeFrom(box.axis, extents[index], extent)};

                cursor = clampToU32(
                    std::uint64_t{cursor} + extents[index] + box.gap);
            }
        }

        void arrangeChildren(LayoutTree &tree, std::size_t index)
        {
            std::vector<std::size_t> children;

            for (auto child = tree.node(index).firstChild;
                 child != kNoNode;
                 child = tree.node(child).nextSibling)
            {
                children.push_back(child);
            }

            if (children.empty())
            {
                return;
            }

            const auto &parent = tree.node(index);
            const auto axis = parent.axis;
            const auto inset = doubled(parent.padding);
            const auto pad = static_cast<std::int32_t>(parent.padding);

            const ContentBox box{
                .origin =
                    {.x = parent.arranged.origin.x + pad,
                     .y = parent.arranged.origin.y + pad},
                .size =
                    {.width =
                         saturatingSub(parent.arranged.size.width, inset),
                     .height = saturatingSub(
                         parent.arranged.size.height, inset)},
                .axis = axis,
                .cross = parent.cross,
                .gap = parent.gap};

            const auto count =
                static_cast<std::uint32_t>(children.size());
            const auto gaps =
                clampToU32(std::uint64_t{box.gap} * (count - 1));
            const auto room = saturatingSub(mainOf(box.size, axis), gaps);

            std::vector<std::uint32_t> extents;
            extents.reserve(children.size());

            std::uint64_t demand = 0;
            std::uint32_t growers = 0;

            for (const auto child : children)
            {
                const auto &value = tree.node(child);
                const auto base = mainDemand(value, axis);

                extents.push_back(base);
                demand += base;

                if (mainSizing(value, axis).mode == SizeMode::Grow)
                {
                    ++growers;
                }
            }

            if (demand < room && growers > 0)
            {
                distributeGrowth(
                    tree,
                    children,
                    extents,
                    axis,
                    static_cast<std::uint32_t>(room - demand),
                    growers);
            }
            else if (demand > room)
            {
                distributeShrink(extents, demand, room);
            }

            place(tree, children, extents, box);
        }
    } // namespace

    void layout(LayoutTree &tree, Size canvas)
    {
        measure(tree);

        tree.node(0).arranged =
            Rect{.origin = {.x = 0, .y = 0}, .size = canvas};

        for (std::size_t index = 0; index < tree.size(); ++index)
        {
            arrangeChildren(tree, index);
        }
    }

} // namespace antwika::ui::detail
