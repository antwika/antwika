#include <gtest/gtest.h>

#include <antwika/autotile/TileDraw.hpp>
#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

using antwika::autotile::DrawKind;
using antwika::autotile::TileDraw;
using antwika::tilemap::TerrainClass;

namespace
{
    /**
     * @brief Checks that one member takes part in the comparison.
     */
    template <typename Mutate>
    void expectMemberCompared(const TileDraw &base, Mutate mutate)
    {
        auto changed = base;
        mutate(changed);

        EXPECT_NE(base, changed);
        EXPECT_EQ(base, base);
    }

    [[nodiscard]] TileDraw drawOf()
    {
        return TileDraw{
            .terrain = TerrainClass::Water,
            .kind = DrawKind::Sprite,
            .atlasRow = 7,
            .frame = 2,
            .screen = {.x = 16, .y = 24}};
    }
}

TEST(TileDrawTest, DrawKind_CountsFiveKinds)
{
    EXPECT_EQ(antwika::enums::kCount<DrawKind>, 5U);
}

TEST(TileDrawTest, TileDraw_DefaultsToAFloorSpriteAtTheOrigin)
{
    const TileDraw draw;

    EXPECT_EQ(draw.terrain, TerrainClass::Floor);
    EXPECT_EQ(draw.kind, DrawKind::Sprite);
    EXPECT_EQ(draw.atlasRow, 0);
    EXPECT_EQ(draw.frame, 0);
    EXPECT_EQ(draw.screen.x, 0);
    EXPECT_EQ(draw.screen.y, 0);
}

TEST(TileDrawTest, OperatorEquals_ComparesEveryField)
{
    const auto base = drawOf();

    EXPECT_EQ(base, drawOf());
    expectMemberCompared(
        base, [](auto &one) { one.terrain = TerrainClass::Cliff; });
    expectMemberCompared(
        base, [](auto &one) { one.kind = DrawKind::Shade; });
    expectMemberCompared(base, [](auto &one) { one.atlasRow = 8; });
    expectMemberCompared(base, [](auto &one) { one.frame = 3; });
    expectMemberCompared(base, [](auto &one) { one.screen.x = 32; });
    expectMemberCompared(base, [](auto &one) { one.screen.y = 32; });
}
