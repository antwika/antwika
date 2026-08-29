#include "LayoutTree.hpp"

#include <cstddef>
#include <utility>

namespace antwika::ui::detail
{

    LayoutTree::LayoutTree(Node rootNode)
    {
        nodes.push_back(std::move(rootNode));
    }

    std::size_t LayoutTree::getSize() const noexcept
    {
        return nodes.size();
    }

    Node &LayoutTree::getNode(std::size_t index)
    {
        return nodes[index];
    }

    const Node &LayoutTree::getNode(std::size_t index) const
    {
        return nodes[index];
    }

    std::size_t LayoutTree::getOpenIndex() const noexcept
    {
        return openNode;
    }

    std::size_t LayoutTree::open(Node containerNode)
    {
        openNode = append(std::move(containerNode));

        return openNode;
    }

    void LayoutTree::close() noexcept
    {
        openNode = nodes[openNode].parent;
    }

    std::size_t LayoutTree::add(Node leafNode)
    {
        return append(std::move(leafNode));
    }

    void LayoutTree::addArea(Area area)
    {
        areaList.push_back(std::move(area));
    }

    const std::vector<Area> &LayoutTree::getAreas() const noexcept
    {
        return areaList;
    }

    void LayoutTree::addRail(ScrollBar railBar)
    {
        scrollBars.push_back(railBar);
    }

    const std::vector<ScrollBar> &LayoutTree::getRails() const noexcept
    {
        return scrollBars;
    }

    void LayoutTree::addBar(Splitter barSplitter)
    {
        splitters.push_back(barSplitter);
    }

    const std::vector<Splitter> &LayoutTree::getBars() const noexcept
    {
        return splitters;
    }

    void LayoutTree::addEdge(PanelEdge panelEdge)
    {
        panelEdges.push_back(panelEdge);
    }

    const std::vector<PanelEdge> &LayoutTree::getEdges() const noexcept
    {
        return panelEdges;
    }

    void LayoutTree::addPane(ScrollPane pane)
    {
        scrollPanes.push_back(pane);
    }

    const std::vector<ScrollPane> &LayoutTree::getPanes() const noexcept
    {
        return scrollPanes;
    }

    std::size_t LayoutTree::append(Node valueNode)
    {
        valueNode.parent = openNode;
        nodes.push_back(std::move(valueNode));

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
