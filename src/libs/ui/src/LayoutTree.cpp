#include "LayoutTree.hpp"

#include <cstddef>
#include <utility>

namespace antwika::ui::detail
{

    LayoutTree::LayoutTree(Node root)
    {
        nodes.push_back(std::move(root));
    }

    std::size_t LayoutTree::size() const noexcept
    {
        return nodes.size();
    }

    Node &LayoutTree::node(std::size_t index)
    {
        return nodes[index];
    }

    const Node &LayoutTree::node(std::size_t index) const
    {
        return nodes[index];
    }

    std::size_t LayoutTree::openIndex() const noexcept
    {
        return openNode;
    }

    std::size_t LayoutTree::open(Node container)
    {
        openNode = append(std::move(container));

        return openNode;
    }

    void LayoutTree::close() noexcept
    {
        openNode = nodes[openNode].parent;
    }

    std::size_t LayoutTree::add(Node leaf)
    {
        return append(std::move(leaf));
    }

    void LayoutTree::addArea(Area area)
    {
        areaList.push_back(area);
    }

    const std::vector<Area> &LayoutTree::areas() const noexcept
    {
        return areaList;
    }

    void LayoutTree::addRail(Rail rail)
    {
        railList.push_back(rail);
    }

    const std::vector<Rail> &LayoutTree::rails() const noexcept
    {
        return railList;
    }

    void LayoutTree::addBar(Bar bar)
    {
        barList.push_back(bar);
    }

    const std::vector<Bar> &LayoutTree::bars() const noexcept
    {
        return barList;
    }

    std::size_t LayoutTree::append(Node value)
    {
        value.parent = openNode;
        nodes.push_back(std::move(value));

        const auto index = nodes.size() - 1;

        auto &parent = nodes[openNode];

        if (parent.firstChild == kNoNode)
        {
            parent.firstChild = index;
        }
        else
        {
            nodes[parent.lastChild].nextSibling = index;
        }

        parent.lastChild = index;

        return index;
    }

}
