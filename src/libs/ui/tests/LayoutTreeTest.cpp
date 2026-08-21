#include <gtest/gtest.h>

#include <cstddef>

#include "antwika/ui/Axis.hpp"

#include "LayoutTree.hpp"
#include "Node.hpp"
#include "NodeKind.hpp"

using antwika::ui::Axis;
using antwika::ui::detail::kNoNode;
using antwika::ui::detail::LayoutTree;
using antwika::ui::detail::Node;
using antwika::ui::detail::NodeKind;

namespace
{
    Node root()
    {
        return Node{.axis = Axis::Column};
    }
}

TEST(LayoutTreeTest, Ctor_HoldsJustTheRoot)
{
    const LayoutTree tree{root()};

    EXPECT_EQ(1U, tree.size());
    EXPECT_EQ(kNoNode, tree.node(0).parent);
    EXPECT_EQ(kNoNode, tree.node(0).firstChild);
}

TEST(LayoutTreeTest, Ctor_OpensTheRoot)
{
    const LayoutTree tree{root()};

    EXPECT_EQ(0U, tree.openIndex());
}

TEST(LayoutTreeTest, Add_AppendsAsAChildOfTheOpenContainer)
{
    LayoutTree tree{root()};

    const auto leaf = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(1U, leaf);
    EXPECT_EQ(2U, tree.size());
    EXPECT_EQ(0U, tree.node(leaf).parent);
    EXPECT_EQ(leaf, tree.node(0).firstChild);
    EXPECT_EQ(leaf, tree.node(0).lastChild);
    EXPECT_EQ(kNoNode, tree.node(leaf).nextSibling);
}

TEST(LayoutTreeTest, Add_LinksASecondChildToTheFirst)
{
    LayoutTree tree{root()};

    const auto first = tree.add(Node{.kind = NodeKind::Text});
    const auto second = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(first, tree.node(0).firstChild);
    EXPECT_EQ(second, tree.node(0).lastChild);
    EXPECT_EQ(second, tree.node(first).nextSibling);
    EXPECT_EQ(kNoNode, tree.node(second).nextSibling);
}

TEST(LayoutTreeTest, Open_MakesTheNewContainerTheOpenOne)
{
    LayoutTree tree{root()};

    const auto openedNode = tree.open(Node{.axis = Axis::Row});

    EXPECT_EQ(openedNode, tree.openIndex());
    EXPECT_EQ(0U, tree.node(openedNode).parent);
}

TEST(LayoutTreeTest, Open_TakesSubsequentChildren)
{
    LayoutTree tree{root()};

    const auto openedNode = tree.open(Node{.axis = Axis::Row});
    const auto leaf = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(openedNode, tree.node(leaf).parent);
    EXPECT_EQ(leaf, tree.node(openedNode).firstChild);
    EXPECT_EQ(kNoNode, tree.node(0).nextSibling);
}

TEST(LayoutTreeTest, Close_GoesBackToTheParentContainer)
{
    LayoutTree tree{root()};

    tree.open(Node{.axis = Axis::Row});
    tree.close();

    EXPECT_EQ(0U, tree.openIndex());
}

TEST(LayoutTreeTest, Close_LetsTheNextChildBecomeASibling)
{
    LayoutTree tree{root()};

    const auto first = tree.open(Node{.axis = Axis::Row});
    tree.close();
    const auto second = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(0U, tree.node(second).parent);
    EXPECT_EQ(second, tree.node(first).nextSibling);
}

TEST(LayoutTreeTest, Append_AlwaysGivesAChildALargerIndexThanItsParent)
{
    LayoutTree tree{root()};

    tree.open(Node{.axis = Axis::Row});
    tree.add(Node{.kind = NodeKind::Text});
    tree.open(Node{.axis = Axis::Column});
    tree.add(Node{.kind = NodeKind::Text});

    for (std::size_t index = 1; index < tree.size(); ++index)
    {
        EXPECT_LT(tree.node(index).parent, index);
    }
}
