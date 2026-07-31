#include <cstdint>

#include <gtest/gtest.h>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/CompanionError.hpp"
#include "antwika/companion/Pet.hpp"

using antwika::companion::CompanionError;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetState;
using antwika::time::Tick;

namespace
{
    // A long day, so the night never interrupts a hunger trace.
    // Its numbers are small multiples of each other, not the shipped ones.
    // So a trace through it can be read in one sitting.
    constexpr PetConfig kLongDay{
        .dayTicks = 20,
        .nightTicks = 8,
        .hungerPeriodTicks = 2,
        .starvePeriodTicks = 3,
        .restPeriodTicks = 2,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 2,
        .happinessMax = 6,
        .happinessStart = 4};

    // A day short enough to reach the night in three steps.
    // Both hunger periods are pushed out of reach.
    // So only the night is under test here.
    constexpr PetConfig kShortDay{
        .dayTicks = 4,
        .nightTicks = 6,
        .hungerPeriodTicks = 1000,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 2,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 2,
        .happinessMax = 6,
        .happinessStart = 4};

    void stepTimes(Pet &pet, const Tick times)
    {
        for (Tick step = 0; step < times; ++step)
        {
            pet.step();
        }
    }

    TEST(PetTest, Construction_StartsAwakeFedAndReasonablyHappy)
    {
        const Pet pet(kLongDay);

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.hunger(), 0U);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart);
        EXPECT_EQ(pet.ticks(), 0U);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.disturbances(), 0U);
        EXPECT_FALSE(pet.hungry());
        EXPECT_FALSE(pet.night());
        EXPECT_FALSE(pet.disturbed());
    }

    TEST(PetTest, Construction_RefusesNumbersNoSessionCouldRunOn)
    {
        EXPECT_THROW(Pet(PetConfig{.dayTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.nightTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.hungerPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.starvePeriodTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.restPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.hungerMax = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.happinessMax = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.happinessStart = 0}), CompanionError);
    }

    TEST(PetTest, Settings_AreTheNumbersItWasBuiltWith)
    {
        const Pet pet(kLongDay);
        EXPECT_EQ(pet.settings().hungerMax, kLongDay.hungerMax);
        EXPECT_EQ(pet.settings().happinessMax, kLongDay.happinessMax);
    }

    TEST(PetTest, Step_GetsHungrierOneStepEveryHungerPeriod)
    {
        Pet pet(kLongDay);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.hunger(), 0U);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.hunger(), 1U);
        EXPECT_FALSE(pet.hungry());

        stepTimes(pet, 2);
        EXPECT_EQ(pet.hunger(), 2U);
        EXPECT_TRUE(pet.hungry());
    }

    TEST(PetTest, Step_HungerStopsAtItsMaximum)
    {
        Pet pet(kLongDay);

        stepTimes(pet, 8);
        EXPECT_EQ(pet.hunger(), kLongDay.hungerMax);

        // Two more hunger periods, with nothing left to add.
        stepTimes(pet, 4);
        EXPECT_EQ(pet.hunger(), kLongDay.hungerMax);
    }

    TEST(PetTest, Step_BeingFamishedCostsHappinessEveryStarvePeriod)
    {
        Pet pet(kLongDay);

        // Hunger reaches its maximum on the eighth tick.
        // The ninth is the first starve period after that.
        stepTimes(pet, 8);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart - 1);

        stepTimes(pet, 3);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart - 2);
    }

    TEST(PetTest, Step_NeglectIsWhatEventuallyEndsIt)
    {
        Pet pet(kLongDay);

        // Four starve periods after hunger tops out at tick eight.
        // Four is exactly the happiness it started with.
        stepTimes(pet, 17);
        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.happiness(), 1U);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(pet.happiness(), 0U);
    }

    TEST(PetTest, Step_AfterPerishingOnlyTheClockMoves)
    {
        Pet pet(kLongDay);
        stepTimes(pet, 18);
        ASSERT_EQ(pet.state(), PetState::Perished);

        const auto hunger = pet.hunger();
        stepTimes(pet, 10);

        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(pet.happiness(), 0U);
        EXPECT_EQ(pet.hunger(), hunger);
        EXPECT_EQ(pet.ticks(), 28U);

        // The sun does not stop for it.
        // The day is the tick count, not anything that can have happened.
        EXPECT_FALSE(pet.night());
    }

    TEST(PetTest, Tap_WhileAwakeAndHungryFeedsItAndCheersItUp)
    {
        Pet pet(kLongDay);
        stepTimes(pet, 6);
        ASSERT_EQ(pet.hunger(), 3U);

        pet.tap();

        EXPECT_EQ(pet.hunger(), 1U);
        EXPECT_EQ(pet.meals(), 1U);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart + 1);
    }

    TEST(PetTest, Tap_AMealNeverLeavesHungerBelowNothing)
    {
        Pet pet(kLongDay);
        stepTimes(pet, 4);
        ASSERT_EQ(pet.hunger(), kLongDay.feedRelief);

        pet.tap();
        EXPECT_EQ(pet.hunger(), 0U);
    }

    TEST(PetTest, Tap_WhileAwakeAndNotHungryChangesNothingAtAll)
    {
        Pet pet(kLongDay);
        stepTimes(pet, 2);
        ASSERT_FALSE(pet.hungry());

        pet.tap();

        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.hunger(), 1U);
        EXPECT_EQ(pet.happiness(), kLongDay.happinessStart);
    }

    TEST(PetTest, Step_FallsAsleepAtDuskAndWakesAtDawn)
    {
        Pet pet(kShortDay);

        stepTimes(pet, 3);
        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_FALSE(pet.night());

        stepTimes(pet, 1);
        EXPECT_EQ(pet.state(), PetState::Asleep);
        EXPECT_TRUE(pet.night());

        stepTimes(pet, 6);
        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_FALSE(pet.night());
    }

    TEST(PetTest, Step_AnUndisturbedNightGivesHappinessBack)
    {
        Pet pet(kShortDay);

        stepTimes(pet, 4);
        EXPECT_EQ(pet.happiness(), kShortDay.happinessStart + 1);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.happiness(), kShortDay.happinessStart + 1);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.happiness(), kShortDay.happinessStart + 2);
    }

    TEST(PetTest, Step_HappinessNeverPassesItsMaximum)
    {
        Pet pet(kShortDay);

        stepTimes(pet, 40);
        EXPECT_EQ(pet.happiness(), kShortDay.happinessMax);
    }

    TEST(PetTest, Tap_WhileAsleepCostsHappinessAndTheRestOfTheNight)
    {
        Pet pet(kShortDay);
        stepTimes(pet, 4);
        const auto rested = pet.happiness();
        ASSERT_EQ(pet.state(), PetState::Asleep);

        pet.tap();

        EXPECT_EQ(pet.happiness(), rested - kShortDay.disturbCost);
        EXPECT_EQ(pet.disturbances(), 1U);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_TRUE(pet.disturbed());

        // Two more rest periods, and no rest from either of them.
        const auto disturbedTo = pet.happiness();
        stepTimes(pet, 4);
        EXPECT_EQ(pet.happiness(), disturbedTo);
    }

    TEST(PetTest, Step_ANewNightForgetsTheLastOnesInterruption)
    {
        Pet pet(kShortDay);
        stepTimes(pet, 4);
        pet.tap();
        const auto disturbedTo = pet.happiness();

        // Through the rest of the night and the whole of the next day.
        // Then one rest period into the night after that.
        stepTimes(pet, 10);

        EXPECT_FALSE(pet.disturbed());
        EXPECT_EQ(pet.state(), PetState::Asleep);
        EXPECT_EQ(pet.happiness(), disturbedTo + 1);
    }

    TEST(PetTest, Tap_WhileAsleepCanBeWhatEndsIt)
    {
        PetConfig fragile = kShortDay;
        fragile.happinessStart = 2;
        fragile.restPeriodTicks = 1000;

        Pet pet(fragile);
        stepTimes(pet, 4);
        ASSERT_EQ(pet.state(), PetState::Asleep);

        pet.tap();

        EXPECT_EQ(pet.happiness(), 0U);
        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(PetTest, Tap_AfterPerishingDoesNothing)
    {
        Pet pet(kLongDay);
        stepTimes(pet, 18);
        ASSERT_EQ(pet.state(), PetState::Perished);

        pet.tap();

        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.disturbances(), 0U);
        EXPECT_EQ(pet.happiness(), 0U);
        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    // The shipped numbers, rather than the readable ones above.
    // A companion left entirely alone has to die of it.
    // One fed on time has to be fine.
    // Otherwise the balance is not a game.
    TEST(PetTest, Step_TheShippedNumbersKillAnUnattendedCompanion)
    {
        Pet neglected;
        for (Tick step = 0; step < 100 * antwika::companion::kTicksPerSecond
             && neglected.state() != PetState::Perished;
             ++step)
        {
            neglected.step();
        }

        EXPECT_EQ(neglected.state(), PetState::Perished);
    }

    TEST(PetTest, Step_TheShippedNumbersKeepAWellFedCompanionAlive)
    {
        Pet kept;
        for (Tick step = 0; step < 100 * antwika::companion::kTicksPerSecond;
             ++step)
        {
            kept.step();

            if (kept.state() == PetState::Awake && kept.hungry())
            {
                kept.tap();
            }
        }

        EXPECT_NE(kept.state(), PetState::Perished);
        EXPECT_GT(kept.meals(), 0U);
        EXPECT_EQ(kept.disturbances(), 0U);
    }
} // namespace
