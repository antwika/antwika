#pragma once

#include <cstddef>
#include <vector>

#include "Area.hpp"
#include "Bar.hpp"
#include "Node.hpp"
#include "Rail.hpp"

namespace antwika::ui::detail
{

    class LayoutTree final
    {
    public:
        explicit LayoutTree(Node root);

        [[nodiscard]] std::size_t size() const noexcept;

        [[nodiscard]] Node &node(std::size_t index);

        [[nodiscard]] const Node &node(std::size_t index) const;

        [[nodiscard]] std::size_t openIndex() const noexcept;

        std::size_t open(Node container);

        void close() noexcept;

        std::size_t add(Node leaf);

        void addArea(Area area);

        [[nodiscard]] const std::vector<Area> &areas() const noexcept;

        void addRail(Rail rail);

        [[nodiscard]] const std::vector<Rail> &rails() const noexcept;

        void addBar(Bar bar);

        [[nodiscard]] const std::vector<Bar> &bars() const noexcept;

    private:
        std::size_t append(Node value);

        std::vector<Node> nodes;
        std::vector<Area> areaList;
        std::vector<Rail> railList;
        std::vector<Bar> barList;
        std::size_t openNode = 0;
    };

}
