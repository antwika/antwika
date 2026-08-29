#include <gtest/gtest.h>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/Inventory.hpp>
#include <antwika/component/Patrol.hpp>
#include <antwika/component/Player.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/component/Speaker.hpp>
#include <antwika/component/Velocity.hpp>

namespace
{

    using namespace antwika::component;

    template <typename Held, typename Member>
    void expectMemberTells(
        Held heldValue, Member Held::*member, Member otherMember)
    {
        const Held heldTwin = heldValue;

        EXPECT_EQ(heldValue, heldTwin);

        heldValue.*member = otherMember;

        EXPECT_NE(heldValue, heldTwin);
    }

}

TEST(EqualityTest, Position_TellsEachAxisApart)
{
    expectMemberTells(Position{}, &Position::x, 1.0F);
    expectMemberTells(Position{}, &Position::y, 1.0F);
    expectMemberTells(Position{}, &Position::z, 1.0F);
}

TEST(EqualityTest, Velocity_TellsEachPartApart)
{
    expectMemberTells(Velocity{}, &Velocity::velocityX, 1.0F);
    expectMemberTells(Velocity{}, &Velocity::velocityZ, 1.0F);
    expectMemberTells(Velocity{}, &Velocity::speedMultiplier, 2.0F);
}

TEST(EqualityTest, AnimationState_TellsEachPartApart)
{
    expectMemberTells(
        AnimationState{}, &AnimationState::direction, std::uint8_t{3});
    expectMemberTells(AnimationState{}, &AnimationState::walking, true);
    expectMemberTells(
        AnimationState{},
        &AnimationState::startedAtTick,
        decltype(AnimationState{}.startedAtTick){7});
}

TEST(EqualityTest, Health_TellsFoodFromWater)
{
    expectMemberTells(Health{}, &Health::food, std::uint16_t{7});
    expectMemberTells(Health{}, &Health::water, std::uint16_t{9});
}

TEST(EqualityTest, Inventory_TellsTheSlotsApart)
{
    expectMemberTells(
        Inventory{},
        &Inventory::slots,
        std::array<std::uint8_t, 4>{1, 0, 0, 0});
}

TEST(EqualityTest, Patrol_TellsStopFromPath)
{
    expectMemberTells(Patrol{}, &Patrol::nextStopIndex, std::uint32_t{2});
    expectMemberTells(Patrol{}, &Patrol::pathIndex, std::uint32_t{3});
}

TEST(EqualityTest, Speaker_TellsTheNextLine)
{
    expectMemberTells(Speaker{}, &Speaker::nextLineIndex, std::uint32_t{4});
}

TEST(EqualityTest, Player_TellsThePaddingByte)
{
    expectMemberTells(Player{}, &Player::padding, std::uint8_t{1});
}

TEST(EqualityTest, CharacterIndex_TellsTheIndex)
{
    expectMemberTells(CharacterIndex{}, &CharacterIndex::index, std::uint32_t{5});
}

TEST(EqualityTest, CarriedLight_TellsEachPartApart)
{
    expectMemberTells(
        CarriedLight{},
        &CarriedLight::tintColor,
        antwika::gfx::Color{.red = 1});
    expectMemberTells(CarriedLight{}, &CarriedLight::aboveHeight, 9.0F);
    expectMemberTells(CarriedLight{}, &CarriedLight::reach, 9.0F);
}

TEST(EqualityTest, FillLight_TellsEachPartApart)
{
    expectMemberTells(
        FillLight{}, &FillLight::tintColor, antwika::gfx::Color{.red = 1});
    expectMemberTells(FillLight{}, &FillLight::aboveHeight, 9.0F);
    expectMemberTells(FillLight{}, &FillLight::reach, 9.0F);
}
