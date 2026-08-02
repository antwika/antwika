#pragma once

#include <cstddef>
#include <vector>

#include "Area.hpp"
#include "Node.hpp"

namespace antwika::ui::detail
{

    /**
     * @brief The arena one frame's widgets are appended to.
     *
     * A flat vector with parent and sibling indices rather than nodes
     * owning their children.
     * Appending in the order the caller declares widgets means a child's
     * index is always greater than its parent's, and siblings appear in
     * ascending order, so measuring is one descending loop, arranging is
     * one ascending loop, and painting is the ascending order too.
     * No recursion, no explicit stack, and no depth a deeply nested
     * layout could exceed.
     */
    class LayoutTree final
    {
    public:
        /**
         * @brief Start an arena holding just a root container.
         * @param root The root, which every other node descends from.
         */
        explicit LayoutTree(Node root);

        /**
         * @brief Get how many nodes the arena holds.
         * @return The count, never zero: the root is always present.
         */
        [[nodiscard]] std::size_t size() const noexcept;

        /**
         * @brief Get one node for reading and writing.
         * @param index The node's index, which must be below size().
         * @return The node.
         */
        [[nodiscard]] Node &node(std::size_t index);

        /**
         * @brief Get one node for reading.
         * @param index The node's index, which must be below size().
         * @return The node.
         */
        [[nodiscard]] const Node &node(std::size_t index) const;

        /**
         * @brief Get the container children are currently appended to.
         * @return Its index; the root's until something is opened.
         */
        [[nodiscard]] std::size_t openIndex() const noexcept;

        /**
         * @brief Append a container and start filling it.
         * @param container The container to append.
         * @return Its index.
         */
        std::size_t open(Node container);

        /**
         * @brief Finish the open container and go back to its parent.
         *
         * Must not be called while the root is the open container, since
         * the root has no parent to go back to.
         * Nothing can call it then: the only caller is a Scope, and a
         * Scope only exists for a container that open() returned.
         */
        void close() noexcept;

        /**
         * @brief Append a node without opening it.
         * @param leaf The node to append.
         * @return Its index.
         */
        std::size_t add(Node leaf);

        /**
         * @brief Note a text area, for resolving once this is laid out.
         *
         * Kept beside the nodes rather than in a list of its own,
         * because it is exactly as long-lived as they are: one frame.
         *
         * @param area What the pointer will have to be resolved
         * against.
         */
        void addArea(Area area);

        /**
         * @brief Get this frame's text areas, in declaration order.
         * @return The areas, empty for a frame that declared none.
         */
        [[nodiscard]] const std::vector<Area> &areas() const noexcept;

    private:
        std::size_t append(Node value);

        std::vector<Node> nodes;
        std::vector<Area> areaList;
        std::size_t openNode = 0;
    };

} // namespace antwika::ui::detail
