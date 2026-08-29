#pragma once

#include <cstddef>
#include <vector>

#include "Area.hpp"
#include "PanelEdge.hpp"
#include "Splitter.hpp"
#include "Node.hpp"
#include "ScrollBar.hpp"
#include "ScrollPane.hpp"

namespace antwika::ui::detail
{

    class LayoutTree final
    {
    public:
        explicit LayoutTree(Node rootNode);

        [[nodiscard]] std::size_t getSize() const noexcept;

        [[nodiscard]] Node &getNode(std::size_t index);

        [[nodiscard]] const Node &getNode(std::size_t index) const;

        [[nodiscard]] std::size_t getOpenIndex() const noexcept;

        std::size_t open(Node containerNode);

        void close() noexcept;

        std::size_t add(Node leafNode);

        void addArea(Area area);

        [[nodiscard]] const std::vector<Area> &getAreas() const noexcept;

        void addRail(ScrollBar railBar);

        [[nodiscard]] const std::vector<ScrollBar> &getRails() const noexcept;

        void addBar(Splitter barSplitter);

        [[nodiscard]] const std::vector<Splitter> &getBars() const noexcept;

        void addEdge(PanelEdge panelEdge);

        [[nodiscard]] const std::vector<PanelEdge> &getEdges()
            const noexcept;

        void addPane(ScrollPane pane);

        [[nodiscard]] const std::vector<ScrollPane> &getPanes()
            const noexcept;

    private:
        std::size_t append(Node valueNode);

        std::vector<Node> nodes;
        std::vector<Area> areaList;
        std::vector<ScrollBar> scrollBars;
        std::vector<Splitter> splitters;
        std::vector<PanelEdge> panelEdges;
        std::vector<ScrollPane> scrollPanes;
        std::size_t openNode = 0;
    };

}
