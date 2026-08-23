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
    Node getRoot()
    {
        return Node{.axis = Axis::Column};
    }
}

TEST(LayoutTreeTest, Ctor_HoldsJustTheRoot)
{
    const LayoutTree tree{getRoot()};

    EXPECT_EQ(1U, tree.getSize());
    EXPECT_EQ(kNoNode, tree.getNode(0).parent);
    EXPECT_EQ(kNoNode, tree.getNode(0).firstChild);
}

TEST(LayoutTreeTest, Ctor_OpensTheRoot)
{
    const LayoutTree tree{getRoot()};

    EXPECT_EQ(0U, tree.getOpenIndex());
}

TEST(LayoutTreeTest, Add_AppendsAsAChildOfTheOpenContainer)
{
    LayoutTree tree{getRoot()};

    const auto leaf = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(1U, leaf);
    EXPECT_EQ(2U, tree.getSize());
    EXPECT_EQ(0U, tree.getNode(leaf).parent);
    EXPECT_EQ(leaf, tree.getNode(0).firstChild);
    EXPECT_EQ(leaf, tree.getNode(0).lastChild);
    EXPECT_EQ(kNoNode, tree.getNode(leaf).nextSibling);
}

TEST(LayoutTreeTest, Add_LinksASecondChildToTheFirst)
{
    LayoutTree tree{getRoot()};

    const auto first = tree.add(Node{.kind = NodeKind::Text});
    const auto second = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(first, tree.getNode(0).firstChild);
    EXPECT_EQ(second, tree.getNode(0).lastChild);
    EXPECT_EQ(second, tree.getNode(first).nextSibling);
    EXPECT_EQ(kNoNode, tree.getNode(second).nextSibling);
}

TEST(LayoutTreeTest, Open_MakesTheNewContainerTheOpenOne)
{
    LayoutTree tree{getRoot()};

    const auto openedNode = tree.open(Node{.axis = Axis::Row});

    EXPECT_EQ(openedNode, tree.getOpenIndex());
    EXPECT_EQ(0U, tree.getNode(openedNode).parent);
}

TEST(LayoutTreeTest, Open_TakesSubsequentChildren)
{
    LayoutTree tree{getRoot()};

    const auto openedNode = tree.open(Node{.axis = Axis::Row});
    const auto leaf = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(openedNode, tree.getNode(leaf).parent);
    EXPECT_EQ(leaf, tree.getNode(openedNode).firstChild);
    EXPECT_EQ(kNoNode, tree.getNode(0).nextSibling);
}

TEST(LayoutTreeTest, Close_GoesBackToTheParentContainer)
{
    LayoutTree tree{getRoot()};

    tree.open(Node{.axis = Axis::Row});
    tree.close();

    EXPECT_EQ(0U, tree.getOpenIndex());
}

TEST(LayoutTreeTest, Close_LetsTheNextChildBecomeASibling)
{
    LayoutTree tree{getRoot()};

    const auto first = tree.open(Node{.axis = Axis::Row});
    tree.close();
    const auto second = tree.add(Node{.kind = NodeKind::Text});

    EXPECT_EQ(0U, tree.getNode(second).parent);
    EXPECT_EQ(second, tree.getNode(first).nextSibling);
}

TEST(LayoutTreeTest, Append_AlwaysGivesAChildALargerIndexThanItsParent)
{
    LayoutTree tree{getRoot()};

    tree.open(Node{.axis = Axis::Row});
    tree.add(Node{.kind = NodeKind::Text});
    tree.open(Node{.axis = Axis::Column});
    tree.add(Node{.kind = NodeKind::Text});

    for (std::size_t index = 1; index < tree.getSize(); ++index)
    {
        EXPECT_LT(tree.getNode(index).parent, index);
    }
}
