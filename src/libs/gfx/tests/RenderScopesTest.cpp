#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>

#include <antwika/gfx/fakes/FakeBareTarget.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/SizeF.hpp>

using antwika::gfx::Point;
using antwika::gfx::PointF;
using antwika::gfx::Rect;
using antwika::gfx::RectF;
using antwika::gfx::Size;
using antwika::gfx::SizeF;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::fakes::FakeBareTarget;
using ::testing::InSequence;
using ::testing::NiceMock;

namespace
{

    constexpr RectF kAreaRect{
        PointF{1.0F, 2.0F}, SizeF{3.0F, 4.0F}};

    constexpr Rect kRegionRect{
        .originPoint = Point{.x = 5, .y = 6},
        .size = Size{.width = 7, .height = 8}};

}

TEST(RenderScopesTest, TargetScope_EndsTheTargetWhenItGoesOutOfScope)
{
    NiceMock<MockRenderer> renderer;
    FakeBareTarget target;
    const InSequence order;

    EXPECT_CALL(renderer, beginTarget(::testing::Ref(target)));
    EXPECT_CALL(renderer, endTarget());

    {
        const auto scope = renderer.targetScope(target);
    }
}

TEST(RenderScopesTest, TargetScope_EndsTheRegionWhenItGoesOutOfScope)
{
    NiceMock<MockRenderer> renderer;
    FakeBareTarget target;
    const InSequence order;

    EXPECT_CALL(
        renderer, beginTargetRegion(::testing::Ref(target), kRegionRect));
    EXPECT_CALL(renderer, endTarget());

    {
        const auto scope = renderer.targetScope(target, kRegionRect);
    }
}

TEST(RenderScopesTest, TargetScope_EndsTheTargetWhenAThrowLeavesTheScope)
{
    NiceMock<MockRenderer> renderer;
    FakeBareTarget target;

    EXPECT_CALL(renderer, endTarget());

    EXPECT_THROW(
        {
            const auto scope = renderer.targetScope(target);

            throw std::runtime_error("the pass gave up part way");
        },
        std::runtime_error);
}

TEST(RenderScopesTest, ClipScope_EndsTheClipWhenItGoesOutOfScope)
{
    NiceMock<MockRenderer> renderer;
    const InSequence order;

    EXPECT_CALL(renderer, beginClip(kAreaRect));
    EXPECT_CALL(renderer, endClip());

    {
        const auto scope = renderer.clipScope(kAreaRect);
    }
}

TEST(RenderScopesTest, ClipScope_EndsTheClipWhenAThrowLeavesTheScope)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, endClip());

    EXPECT_THROW(
        {
            const auto scope = renderer.clipScope(kAreaRect);

            throw std::runtime_error("the panel gave up part way");
        },
        std::runtime_error);
}
