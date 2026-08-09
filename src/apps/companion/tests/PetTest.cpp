#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/CompanionError.hpp"
#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/SaveFormatError.hpp"
#include "antwika/companion/Saying.hpp"

using antwika::companion::CareRecord;
using antwika::companion::CompanionError;
using antwika::companion::DayMood;
using antwika::companion::energyCeilingFor;
using antwika::companion::formFor;
using antwika::companion::LifeStage;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetForm;
using antwika::companion::PetMemory;
using antwika::companion::PetState;
using antwika::companion::SaveFormatError;
using antwika::companion::Saying;
using antwika::companion::stageAt;
using antwika::time::Tick;

namespace
{
    constexpr PetConfig kSimple{
        .hungerPeriodTicks = 4,
        .starvePeriodTicks = 5,
        .funDecayPeriodTicks = 4,
        .fretPeriodTicks = 5,
        .recoverPeriodTicks = 1,
        .restPeriodTicks = 3,
        .sayingTicks = 2,
        .chatterPeriodTicks = 4,
        .drainHappyTicks = 4,
        .drainContentTicks = 3,
        .drainLowTicks = 2,
        .drainMiserableTicks = 1,
        .childTicks = 10000,
        .teenTicks = 20000,
        .adultTicks = 30000,
        .elderTicks = 40000,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .funMax = 4,
        .funStart = 4,
        .playFun = 2,
        .playHunger = 1,
        .playEnergy = 2,
        .playJoy = 1,
        .energyBase = 10,
        .stageEnergyBonus = 4,
        .collapsePenalty = 5,
        .tiredPercent = 40,
        .happinessMax = 6,
        .happinessStart = 4,
        .happyBand = 5,
        .contentBand = 3,
        .disturbCost = 2,
        .pesterCost = 1};

    [[nodiscard]] PetConfig still()
    {
        PetConfig config = kSimple;
        config.hungerPeriodTicks = 10000;
        config.funDecayPeriodTicks = 10000;
        config.starvePeriodTicks = 10000;
        config.fretPeriodTicks = 10000;
        config.drainHappyTicks = 10000;
        config.drainContentTicks = 10000;
        config.drainLowTicks = 10000;
        config.drainMiserableTicks = 10000;
        config.chatterPeriodTicks = 10000;
        return config;
    }

    [[nodiscard]] PetConfig draining()
    {
        PetConfig config = still();
        config.drainHappyTicks = 1;
        config.drainContentTicks = 1;
        config.drainLowTicks = 1;
        config.drainMiserableTicks = 1;
        config.recoverPeriodTicks = 1;
        return config;
    }

    constexpr std::uint32_t kHeavyDay = 2;

    void stepTimes(Pet &pet, const Tick times)
    {
        for (Tick step = 0; step < times; ++step)
        {
            pet.step();
        }
    }

    void runUntilGone(Pet &pet, const Tick limit)
    {
        while (pet.state() != PetState::Perished && pet.ticks() < limit)
        {
            pet.step();
        }
    }

    TEST(PetTest, Ctor_StartsAwakeRestedAndAmused)
    {
        const Pet pet(kSimple);

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.hunger(), 0U);
        EXPECT_EQ(pet.fun(), kSimple.funStart);
        EXPECT_EQ(pet.happiness(), kSimple.happinessStart);
        EXPECT_EQ(pet.energy(), kSimple.energyBase);
        EXPECT_EQ(pet.energyCeiling(), kSimple.energyBase);
        EXPECT_EQ(pet.ticks(), 0U);
        EXPECT_EQ(pet.day(), 0U);
        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.plays(), 0U);
        EXPECT_EQ(pet.disturbances(), 0U);
        EXPECT_EQ(pet.pesters(), 0U);
        EXPECT_EQ(pet.collapses(), 0U);
        EXPECT_FALSE(pet.hungry());
        EXPECT_FALSE(pet.bored());
        EXPECT_FALSE(pet.tired());
        EXPECT_FALSE(pet.night());
        EXPECT_FALSE(pet.disturbed());
        EXPECT_EQ(pet.stage(), LifeStage::Egg);
    }

    TEST(PetTest, Ctor_RefusesNumbersNoSessionCouldRunOn)
    {
        EXPECT_THROW(
            Pet(PetConfig{.hungerPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.starvePeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.funDecayPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.fretPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.recoverPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.restPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.sayingTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.chatterPeriodTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.drainHappyTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.drainContentTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.drainLowTicks = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.drainMiserableTicks = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.hungerMax = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.funMax = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.happinessMax = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.happinessStart = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.energyBase = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.playEnergy = 0}), CompanionError);
        EXPECT_THROW(
            Pet(PetConfig{.collapsePenalty = 0}), CompanionError);
        EXPECT_THROW(Pet(PetConfig{.tiredPercent = 0}), CompanionError);
    }

    TEST(PetTest, Ctor_RefusesABalanceWhereSleepIsFree)
    {
        PetConfig always = kSimple;
        always.tiredPercent = 100;

        EXPECT_THROW((void)Pet(always), CompanionError);
    }

    TEST(PetTest, Ctor_RefusesStartingAboveAMaximum)
    {
        PetConfig tooHappy = kSimple;
        tooHappy.happinessStart = tooHappy.happinessMax + 1;
        EXPECT_THROW((void)Pet(tooHappy), CompanionError);

        PetConfig tooAmused = kSimple;
        tooAmused.funStart = tooAmused.funMax + 1;
        EXPECT_THROW((void)Pet(tooAmused), CompanionError);
    }

    TEST(PetTest, Settings_AreTheNumbersItWasBuiltWith)
    {
        const Pet pet(kSimple);
        EXPECT_EQ(pet.settings().hungerMax, kSimple.hungerMax);
        EXPECT_EQ(pet.settings().funMax, kSimple.funMax);
        EXPECT_EQ(pet.settings().happinessMax, kSimple.happinessMax);
    }

    TEST(PetTest, Step_GetsHungrierOneStepEveryHungerPeriod)
    {
        Pet pet(kSimple);

        stepTimes(pet, 3);
        EXPECT_EQ(pet.hunger(), 0U);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.hunger(), 1U);
        EXPECT_FALSE(pet.hungry());

        stepTimes(pet, 4);
        EXPECT_EQ(pet.hunger(), 2U);
        EXPECT_TRUE(pet.hungry());
    }

    TEST(PetTest, Step_HungerStopsAtItsMaximum)
    {
        Pet pet(kSimple);

        stepTimes(pet, 16);
        EXPECT_EQ(pet.hunger(), kSimple.hungerMax);

        stepTimes(pet, 8);
        EXPECT_EQ(pet.hunger(), kSimple.hungerMax);
    }

    TEST(PetTest, Step_LosesInterestAndStopsAtNone)
    {
        Pet pet(kSimple);

        stepTimes(pet, 4);
        EXPECT_EQ(pet.fun(), kSimple.funStart - 1);
        EXPECT_FALSE(pet.bored());

        stepTimes(pet, 12);
        EXPECT_EQ(pet.fun(), 0U);
        EXPECT_TRUE(pet.bored());

        stepTimes(pet, 4);
        EXPECT_EQ(pet.fun(), 0U);
    }

    TEST(PetTest, Step_HowFastEnergyGoesIsWhichBandItsHappinessIsIn)
    {
        const auto energyAfter =
            [](const std::uint32_t happiness, const Tick ticks)
        {
            PetConfig config = still();
            config.happinessStart = happiness;
            config.drainHappyTicks = 4;
            config.drainContentTicks = 3;
            config.drainLowTicks = 2;

            Pet pet(config);
            stepTimes(pet, ticks);
            return pet.energy();
        };

        EXPECT_EQ(energyAfter(6, 12), 10U - 3U);

        EXPECT_EQ(energyAfter(4, 12), 10U - 4U);

        EXPECT_EQ(energyAfter(2, 12), 10U - 6U);

        PetConfig config = still();
        config.drainMiserableTicks = 1;
        config.happinessStart = 1;
        config.pesterCost = 1;

        Pet pet(config);
        pet.pester();
        ASSERT_EQ(pet.happiness(), 0U);
        stepTimes(pet, 3);
        EXPECT_EQ(pet.energy(), 10U - 3U);
    }

    TEST(PetTest, Step_BeingFamishedCostsHappinessAndEnergyTogether)
    {
        PetConfig config = still();
        config.hungerPeriodTicks = 1;
        config.starvePeriodTicks = 5;

        Pet pet(config);

        stepTimes(pet, 4);
        EXPECT_EQ(pet.happiness(), config.happinessStart);
        EXPECT_EQ(pet.energy(), config.energyBase);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.happiness(), config.happinessStart - 1);
        EXPECT_EQ(pet.energy(), config.energyBase - 1);
    }

    TEST(PetTest, Step_HavingNoFunLeftCostsTheSameTwoThings)
    {
        PetConfig config = still();
        config.funDecayPeriodTicks = 1;
        config.fretPeriodTicks = 5;

        Pet pet(config);

        stepTimes(pet, 4);
        ASSERT_TRUE(pet.bored());
        EXPECT_EQ(pet.happiness(), config.happinessStart);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.happiness(), config.happinessStart - 1);
        EXPECT_EQ(pet.energy(), config.energyBase - 1);
    }

    TEST(PetTest, Step_RunningOutOfEnergyIsACollapseRatherThanAnEnd)
    {
        PetConfig config = still();
        config.drainContentTicks = 1;
        config.recoverPeriodTicks = 10000;

        Pet pet(config);
        stepTimes(pet, 10);

        EXPECT_EQ(pet.state(), PetState::Asleep);
        EXPECT_EQ(pet.energy(), 0U);
        EXPECT_EQ(pet.collapses(), 1U);
        EXPECT_EQ(
            pet.energyCeiling(),
            config.energyBase - config.collapsePenalty);
    }

    TEST(PetTest, Step_TheCollapseThatLeavesNoCeilingIsTheEnd)
    {
        const PetConfig config = draining();

        Pet pet(config);
        runUntilGone(pet, 100);

        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(pet.energy(), 0U);
        EXPECT_EQ(pet.energyCeiling(), 0U);
        EXPECT_EQ(pet.collapses(), 2U);
    }

    TEST(PetTest, Step_OldAgeCanBeWhatFinallyEndsIt)
    {
        PetConfig config = still();
        config.childTicks = 2;
        config.teenTicks = 3;
        config.adultTicks = 4;
        config.elderTicks = 5;
        config.energyBase = 10;
        config.stageEnergyBonus = 5;
        config.collapsePenalty = 5;

        PetMemory memory;
        memory.ticks = 4;
        memory.energy = 5;
        memory.fun = config.funStart;
        memory.happiness = config.happinessStart;
        memory.collapses = 3;

        Pet pet(config, memory);
        ASSERT_EQ(pet.stage(), LifeStage::Adult);
        ASSERT_EQ(pet.energyCeiling(), 10U);
        ASSERT_EQ(pet.state(), PetState::Awake);

        pet.step();

        EXPECT_EQ(pet.stage(), LifeStage::Elder);
        EXPECT_EQ(pet.energyCeiling(), 0U);
        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(PetTest, Step_AfterPerishingOnlyTheClockMoves)
    {
        const PetConfig config = draining();

        Pet pet(config);
        runUntilGone(pet, 100);
        ASSERT_EQ(pet.state(), PetState::Perished);

        const auto at = pet.ticks();
        const auto collapses = pet.collapses();
        stepTimes(pet, 10);

        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(pet.energy(), 0U);
        EXPECT_EQ(pet.collapses(), collapses);
        EXPECT_EQ(pet.ticks(), at + 10);
    }

    TEST(PetTest, Sleep_RecoversEnergyAndWakesWhenItIsFull)
    {
        PetConfig config = still();
        config.recoverPeriodTicks = 2;

        Pet pet(config);

        for (std::uint32_t game = 0; game < 4; ++game)
        {
            pet.play();
        }
        ASSERT_EQ(pet.energy(), 2U);
        ASSERT_TRUE(pet.tired());

        pet.putToBed();
        ASSERT_EQ(pet.state(), PetState::Asleep);

        stepTimes(pet, 2);
        EXPECT_EQ(pet.energy(), 3U);
        EXPECT_EQ(pet.state(), PetState::Asleep);

        stepTimes(pet, 14);
        EXPECT_EQ(pet.energy(), config.energyBase);
        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.day(), 1U);
        EXPECT_FALSE(pet.disturbed());
    }

    TEST(PetTest, Sleep_GivesHappinessBackEveryRestPeriod)
    {
        PetConfig config = still();
        config.recoverPeriodTicks = 10000;
        config.restPeriodTicks = 3;
        config.playJoy = 0;

        Pet pet(config);
        for (std::uint32_t game = 0; game < 4; ++game)
        {
            pet.play();
        }
        const auto cheered = pet.happiness();
        pet.putToBed();
        ASSERT_EQ(pet.state(), PetState::Asleep);

        stepTimes(pet, 3);
        EXPECT_EQ(pet.happiness(), cheered + 1);
    }

    TEST(PetTest, PutToBed_IsRefusedWhileItIsStillWideAwake)
    {
        Pet pet(still());

        pet.putToBed();

        EXPECT_EQ(pet.state(), PetState::Awake);
        EXPECT_EQ(pet.pesters(), 1U);
        EXPECT_EQ(pet.saying(), Saying::NotSleepy);
        EXPECT_EQ(
            pet.happiness(),
            kSimple.happinessStart - kSimple.pesterCost);
    }

    TEST(PetTest, Play_CostsEnergyAndBuysFunAndHunger)
    {
        Pet pet(still());

        pet.play();

        EXPECT_EQ(pet.plays(), 1U);
        EXPECT_EQ(pet.energy(), kSimple.energyBase - kSimple.playEnergy);
        EXPECT_EQ(pet.fun(), kSimple.funMax);
        EXPECT_EQ(pet.hunger(), kSimple.playHunger);
        EXPECT_EQ(pet.saying(), Saying::Wheee);
        EXPECT_EQ(
            pet.happiness(), kSimple.happinessStart + kSimple.playJoy);
    }

    TEST(PetTest, Play_IsRefusedWithTooLittleEnergyToSpend)
    {
        PetConfig config = still();
        config.playEnergy = 4;

        Pet pet(config);
        pet.play();
        pet.play();
        ASSERT_EQ(pet.energy(), 2U);

        pet.play();

        EXPECT_EQ(pet.plays(), 2U);
        EXPECT_EQ(pet.energy(), 2U);
        EXPECT_EQ(pet.pesters(), 1U);
        EXPECT_EQ(pet.saying(), Saying::TooTired);
    }

    TEST(PetTest, Play_SpendingTheVeryLastOfItIsACollapse)
    {
        PetConfig config = still();
        config.playEnergy = 5;

        Pet pet(config);
        pet.play();
        pet.play();

        EXPECT_EQ(pet.energy(), 0U);
        EXPECT_EQ(pet.state(), PetState::Asleep);
        EXPECT_EQ(pet.collapses(), 1U);

        EXPECT_EQ(pet.saying(), Saying::Wheee);
    }

    TEST(PetTest, Feed_WhileHungryFeedsItAndCheersItUp)
    {
        Pet pet(kSimple);
        stepTimes(pet, 12);
        ASSERT_EQ(pet.hunger(), 3U);

        pet.feed();

        EXPECT_EQ(pet.hunger(), 1U);
        EXPECT_EQ(pet.meals(), 1U);
        EXPECT_EQ(pet.saying(), Saying::Yum);
    }

    TEST(PetTest, Feed_AMealNeverLeavesHungerBelowNothing)
    {
        Pet pet(kSimple);
        stepTimes(pet, 8);
        ASSERT_EQ(pet.hunger(), kSimple.feedRelief);

        pet.feed();
        EXPECT_EQ(pet.hunger(), 0U);
    }

    TEST(PetTest, Feed_WhileFullIsAnUnwantedAttention)
    {
        Pet pet(still());
        ASSERT_FALSE(pet.hungry());

        pet.feed();

        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.pesters(), 1U);
        EXPECT_EQ(pet.saying(), Saying::NotHungry);
        EXPECT_EQ(
            pet.happiness(),
            kSimple.happinessStart - kSimple.pesterCost);
    }

    TEST(PetTest, Pester_IsAViolationRatherThanNothingAtAll)
    {
        Pet pet(still());

        pet.pester();

        EXPECT_EQ(pet.pesters(), 1U);
        EXPECT_EQ(pet.saying(), Saying::Poked);
        EXPECT_EQ(
            pet.happiness(),
            kSimple.happinessStart - kSimple.pesterCost);
    }

    TEST(PetTest, Press_WakesASleepingCompanionInstead)
    {
        const auto woken = [](void (Pet::*verb)())
        {
            const PetConfig config = still();
            Pet pet(config);
            for (std::uint32_t game = 0; game < 4; ++game)
            {
                pet.play();
            }
            pet.putToBed();
            const auto before = pet.happiness();
            (pet.*verb)();

            EXPECT_EQ(pet.state(), PetState::Awake);
            EXPECT_EQ(pet.disturbances(), 1U);
            EXPECT_TRUE(pet.disturbed());
            EXPECT_EQ(pet.day(), 1U);
            EXPECT_EQ(pet.saying(), Saying::LetMeSleep);
            EXPECT_EQ(pet.happiness(), before - config.disturbCost);
            return pet.pesters();
        };

        EXPECT_EQ(woken(&Pet::feed), 0U);
        EXPECT_EQ(woken(&Pet::play), 0U);
        EXPECT_EQ(woken(&Pet::putToBed), 0U);
        EXPECT_EQ(woken(&Pet::pester), 0U);
    }

    TEST(PetTest, Press_DoesNothingAfterPerishing)
    {
        const PetConfig config = draining();

        Pet pet(config);
        runUntilGone(pet, 100);
        ASSERT_EQ(pet.state(), PetState::Perished);

        pet.feed();
        pet.play();
        pet.putToBed();
        pet.pester();

        EXPECT_EQ(pet.meals(), 0U);
        EXPECT_EQ(pet.plays(), 0U);
        EXPECT_EQ(pet.pesters(), 0U);
        EXPECT_EQ(pet.disturbances(), 0U);
        EXPECT_EQ(pet.state(), PetState::Perished);
    }

    TEST(PetTest, Saying_StartsWithNothingToSay)
    {
        const Pet pet(kSimple);

        EXPECT_EQ(pet.saying(), Saying::None);
        EXPECT_EQ(pet.sayingTicksLeft(), 0U);
    }

    TEST(PetTest, Saying_FindsSomethingToSayEveryChatterPeriod)
    {
        Pet pet(kSimple);

        stepTimes(pet, 3);
        EXPECT_EQ(pet.saying(), Saying::None);

        stepTimes(pet, 1);
        EXPECT_NE(pet.saying(), Saying::None);
        EXPECT_EQ(pet.sayingTicksLeft(), kSimple.sayingTicks);
    }

    TEST(PetTest, Saying_ForgetsWhatItSaidAfterAWhile)
    {
        Pet pet(kSimple);

        stepTimes(pet, 5);
        EXPECT_NE(pet.saying(), Saying::None);
        EXPECT_EQ(pet.sayingTicksLeft(), 1U);

        stepTimes(pet, 1);
        EXPECT_EQ(pet.saying(), Saying::None);
        EXPECT_EQ(pet.sayingTicksLeft(), 0U);
    }

    TEST(PetTest, Saying_AsksForWhicheverNeedIsOutstanding)
    {
        PetConfig hungry = still();
        hungry.hungerPeriodTicks = 1;
        hungry.chatterPeriodTicks = 4;

        Pet starving(hungry);
        stepTimes(starving, 4);
        EXPECT_TRUE(starving.hungry());
        EXPECT_EQ(starving.saying(), Saying::FeedMe);

        PetConfig dull = still();
        dull.funDecayPeriodTicks = 1;
        dull.chatterPeriodTicks = 4;

        Pet uninterested(dull);
        stepTimes(uninterested, 8);
        EXPECT_TRUE(uninterested.bored());
        EXPECT_EQ(uninterested.saying(), Saying::PlayWithMe);
    }

    TEST(PetTest, Saying_MurmursInItsSleepInstead)
    {
        PetConfig config = still();
        config.recoverPeriodTicks = 10000;
        config.chatterPeriodTicks = 2;

        Pet pet(config);
        for (std::uint32_t game = 0; game < 4; ++game)
        {
            pet.play();
        }
        pet.putToBed();
        ASSERT_EQ(pet.state(), PetState::Asleep);

        stepTimes(pet, 4);
        EXPECT_EQ(pet.saying(), Saying::Zzz);
    }

    TEST(PetTest, Saying_YawnsTheTickItBecomesTiredEnoughForBed)
    {
        PetConfig config = still();
        config.drainContentTicks = 1;
        config.chatterPeriodTicks = 10000;

        Pet pet(config);

        stepTimes(pet, 5);
        ASSERT_FALSE(pet.tired());
        EXPECT_NE(pet.saying(), Saying::Yawn);

        stepTimes(pet, 1);
        EXPECT_TRUE(pet.tired());
        EXPECT_EQ(pet.saying(), Saying::Yawn);
    }

    TEST(PetTest, Saying_AYawnWaitsForWhateverIsAlreadyBeingSaid)
    {
        PetConfig config = still();
        config.drainContentTicks = 1;
        config.chatterPeriodTicks = 10000;
        config.sayingTicks = 10;

        Pet pet(config);
        stepTimes(pet, 5);
        pet.pester();
        ASSERT_EQ(pet.saying(), Saying::Poked);

        stepTimes(pet, 1);
        EXPECT_TRUE(pet.tired());
        EXPECT_EQ(pet.saying(), Saying::Poked);
    }

    TEST(PetTest, Saying_SaysNothingOnceItHasPerished)
    {
        PetConfig config = still();
        config.playEnergy = 5;
        config.energyBase = 5;
        config.collapsePenalty = 5;

        Pet pet(config);
        pet.play();

        EXPECT_EQ(pet.state(), PetState::Perished);
        EXPECT_EQ(pet.saying(), Saying::None);
        EXPECT_EQ(pet.sayingTicksLeft(), 0U);
    }

    TEST(PetTest, Saying_DrawsItsIdleLinesFromMoreThanOne)
    {
        PetConfig config = still();
        config.chatterPeriodTicks = 3;
        config.sayingTicks = 2;

        Pet pet(config);
        std::set<Saying> heard;

        for (Tick step = 0; step < 60; ++step)
        {
            pet.step();

            if (pet.saying() != Saying::None)
            {
                heard.insert(pet.saying());
            }
        }

        EXPECT_GT(heard.size(), 1U);
    }

    TEST(PetTest, Saying_IsAFunctionOfTheTickCountAlone)
    {
        PetConfig config = still();
        config.chatterPeriodTicks = 3;

        Pet first(config);
        stepTimes(first, 12);

        PetMemory otherHistory = first.remember();
        otherHistory.meals = 7;
        otherHistory.plays = 5;
        otherHistory.disturbances = 2;
        otherHistory.pesters = 3;

        Pet second(config, otherHistory);

        for (Tick step = 0; step < 40; ++step)
        {
            first.step();
            second.step();

            ASSERT_EQ(first.saying(), second.saying());
        }
    }

    TEST(PetTest, Stage_IsAFunctionOfTheTicksAndTheCeilingFollowsIt)
    {
        PetConfig config = kSimple;
        config.childTicks = 10;
        config.teenTicks = 20;
        config.adultTicks = 30;
        config.elderTicks = 40;

        EXPECT_EQ(stageAt(config, 0), LifeStage::Egg);
        EXPECT_EQ(stageAt(config, 9), LifeStage::Egg);
        EXPECT_EQ(stageAt(config, 10), LifeStage::Child);
        EXPECT_EQ(stageAt(config, 20), LifeStage::Teen);
        EXPECT_EQ(stageAt(config, 30), LifeStage::Adult);
        EXPECT_EQ(stageAt(config, 40), LifeStage::Elder);

        EXPECT_EQ(energyCeilingFor(config, 0, 0), config.energyBase);
        EXPECT_EQ(
            energyCeilingFor(config, 30, 0),
            config.energyBase + 3 * config.stageEnergyBonus);
        EXPECT_EQ(
            energyCeilingFor(config, 40, 0),
            config.energyBase + config.stageEnergyBonus);

        EXPECT_EQ(energyCeilingFor(config, 0, 100), 0U);
    }

    TEST(PetTest, Form_IsDecidedByWhatItHasBeenThrough)
    {
        EXPECT_EQ(formFor(CareRecord{.meals = 3}), PetForm::Bright);
        EXPECT_EQ(formFor(CareRecord{}), PetForm::Plain);
        EXPECT_EQ(
            formFor(CareRecord{.meals = 4, .pesters = 2}),
            PetForm::Plain);
        EXPECT_EQ(
            formFor(CareRecord{.meals = 1, .collapses = 1}),
            PetForm::Scruffy);
    }

    TEST(PetTest, Care_IsTheCountsItKeeps)
    {
        Pet pet(still());
        pet.play();
        pet.pester();

        const CareRecord care = pet.care();
        EXPECT_EQ(care.plays, 1U);
        EXPECT_EQ(care.pesters, 1U);

        EXPECT_EQ(pet.form(), PetForm::Scruffy);
    }

    TEST(PetTest, Mood_AHeavyDayNeverShortensAPeriodToNothing)
    {
        PetConfig config = still();
        config.drainContentTicks = 1;

        PetMemory memory;
        memory.day = kHeavyDay;
        memory.energy = config.energyBase;
        memory.fun = config.funStart;
        memory.happiness = config.happinessStart;

        Pet pet(config, memory);
        ASSERT_EQ(pet.mood(), DayMood::Heavy);

        stepTimes(pet, 3);

        EXPECT_EQ(pet.energy(), config.energyBase - 3);
    }

    TEST(PetTest, Saying_ACompanionThatDropsAsItTiresSaysNothing)
    {
        PetConfig config = still();
        config.energyBase = 5;
        config.collapsePenalty = 1;
        config.tiredPercent = 20;
        config.drainContentTicks = 1;
        config.hungerPeriodTicks = 1;
        config.hungerMax = 1;
        config.starvePeriodTicks = 1;
        config.funDecayPeriodTicks = 1;
        config.funMax = 1;
        config.funStart = 1;
        config.fretPeriodTicks = 1;

        Pet pet(config);

        stepTimes(pet, 1);
        ASSERT_FALSE(pet.tired());

        stepTimes(pet, 1);
        EXPECT_EQ(pet.state(), PetState::Asleep);
        EXPECT_TRUE(pet.tired());
        EXPECT_NE(pet.saying(), Saying::Yawn);
    }

    TEST(PetTest, Mood_IsAFunctionOfTheDayAndDayZeroIsOrdinary)
    {
        const Pet pet(kSimple);

        EXPECT_EQ(pet.day(), 0U);
        EXPECT_EQ(pet.mood(), DayMood::Ordinary);
    }

    TEST(PetTest, Remember_RoundTripsThroughTheRestoringConstructor)
    {
        Pet lived(kSimple);
        stepTimes(lived, 9);
        lived.feed();
        lived.play();

        const PetMemory memory = lived.remember();
        const Pet resumed(kSimple, memory);

        EXPECT_EQ(resumed.remember(), memory);
        EXPECT_EQ(resumed.ticks(), lived.ticks());
        EXPECT_EQ(resumed.hunger(), lived.hunger());
        EXPECT_EQ(resumed.fun(), lived.fun());
        EXPECT_EQ(resumed.happiness(), lived.happiness());
        EXPECT_EQ(resumed.energy(), lived.energy());
        EXPECT_EQ(resumed.state(), lived.state());
        EXPECT_EQ(resumed.saying(), lived.saying());
        EXPECT_EQ(resumed.sayingTicksLeft(), lived.sayingTicksLeft());
        EXPECT_EQ(resumed.meals(), lived.meals());
        EXPECT_EQ(resumed.plays(), lived.plays());
        EXPECT_EQ(resumed.disturbances(), lived.disturbances());
        EXPECT_EQ(resumed.pesters(), lived.pesters());
        EXPECT_EQ(resumed.collapses(), lived.collapses());
        EXPECT_EQ(resumed.disturbed(), lived.disturbed());
        EXPECT_EQ(resumed.day(), lived.day());
    }

    TEST(PetTest, Ctor_AResumedCompanionCarriesOnWhereItLeftOff)
    {
        Pet lived(kSimple);
        stepTimes(lived, 5);

        Pet resumed(kSimple, lived.remember());

        lived.step();
        resumed.step();

        EXPECT_EQ(resumed.remember(), lived.remember());
    }

    TEST(PetTest, Ctor_RefusesAMemoryBeyondWhatItCouldHold)
    {
        PetMemory hungry;
        hungry.hunger = kSimple.hungerMax + 1;
        EXPECT_THROW((void)Pet(kSimple, hungry), SaveFormatError);

        PetMemory amused;
        amused.fun = kSimple.funMax + 1;
        EXPECT_THROW((void)Pet(kSimple, amused), SaveFormatError);

        PetMemory happy;
        happy.happiness = kSimple.happinessMax + 1;
        EXPECT_THROW((void)Pet(kSimple, happy), SaveFormatError);

        PetMemory rested;
        rested.energy = kSimple.energyBase + 1;
        EXPECT_THROW((void)Pet(kSimple, rested), SaveFormatError);
    }

    TEST(PetTest, Ctor_RefusesAMemoryThatContradictsItself)
    {
        PetMemory perishedWithCeiling;
        perishedWithCeiling.state = PetState::Perished;

        EXPECT_THROW(
            (void)Pet(kSimple, perishedWithCeiling), SaveFormatError);

        PetMemory spentButAlive;
        spentButAlive.collapses = 2;
        spentButAlive.state = PetState::Awake;

        EXPECT_THROW((void)Pet(kSimple, spentButAlive), SaveFormatError);
    }

    TEST(PetTest, Ctor_RefusesAPerishedMemoryMidSentence)
    {
        PetMemory withWords;
        withWords.state = PetState::Perished;
        withWords.collapses = 2;
        withWords.saying = Saying::Zzz;
        withWords.sayingTicksLeft = 60;

        EXPECT_THROW((void)Pet(kSimple, withWords), SaveFormatError);

        PetMemory withCountdown;
        withCountdown.state = PetState::Perished;
        withCountdown.collapses = 2;
        withCountdown.sayingTicksLeft = 60;

        EXPECT_THROW((void)Pet(kSimple, withCountdown), SaveFormatError);
    }

    TEST(PetTest, Ctor_ResumesASilentPerishedCompanion)
    {
        PetMemory memory;
        memory.state = PetState::Perished;
        memory.collapses = 2;
        memory.ticks = 40;
        memory.disturbances = 3;

        Pet resumed(kSimple, memory);

        EXPECT_EQ(resumed.state(), PetState::Perished);
        EXPECT_EQ(resumed.energyCeiling(), 0U);
        EXPECT_EQ(resumed.remember(), memory);
    }

    TEST(PetTest, Ctor_ARestoreStillRefusesUnrunnableNumbers)
    {
        const PetMemory memory;

        PetConfig broken = kSimple;
        broken.hungerPeriodTicks = 0;

        EXPECT_THROW((void)Pet(broken, memory), CompanionError);
    }

    TEST(PetTest, Revive_StartsACompanionWithNoHistoryOfItsOwn)
    {
        const PetConfig config = draining();

        Pet pet(config);
        runUntilGone(pet, 100);
        ASSERT_EQ(pet.state(), PetState::Perished);

        pet.revive();

        EXPECT_EQ(pet.remember(), Pet(config).remember());
        EXPECT_EQ(pet.collapses(), 0U);
        EXPECT_EQ(pet.energy(), config.energyBase);
        EXPECT_EQ(pet.ticks(), 0U);
    }

    TEST(PetTest, Revive_IsLegalOnACompanionThatIsStillWithUs)
    {
        Pet pet(kSimple);
        stepTimes(pet, 2);

        pet.revive();

        EXPECT_EQ(pet.remember(), Pet(kSimple).remember());
    }

    TEST(PetTest, Revive_KeepsTheNumbersItWasBalancedWith)
    {
        Pet pet(kSimple);

        pet.revive();

        EXPECT_EQ(pet.settings().energyBase, kSimple.energyBase);
        EXPECT_EQ(pet.settings().happinessMax, kSimple.happinessMax);
    }

    TEST(PetTest, Step_TheShippedNumbersKillAnUnattendedCompanion)
    {
        Pet neglected;
        for (Tick step = 0;
             step < 600 * antwika::companion::kTicksPerSecond
             && neglected.state() != PetState::Perished;
             ++step)
        {
            neglected.step();
        }

        EXPECT_EQ(neglected.state(), PetState::Perished);
        EXPECT_GT(neglected.collapses(), 1U);
    }

    TEST(PetTest, Step_TheShippedNumbersKeepAnAttendedCompanionAlive)
    {
        Pet kept;
        for (Tick step = 0;
             step < 600 * antwika::companion::kTicksPerSecond;
             ++step)
        {
            kept.step();

            if (kept.state() != PetState::Awake)
            {
                continue;
            }

            if (kept.hungry())
            {
                kept.feed();
            }
            else if (
                2 * kept.fun() <= kept.settings().funMax
                && kept.energy() > kept.energyCeiling() / 2)
            {
                kept.play();
            }
            else if (kept.tired())
            {
                kept.putToBed();
            }
        }

        EXPECT_NE(kept.state(), PetState::Perished);
        EXPECT_EQ(kept.collapses(), 0U);
        EXPECT_EQ(kept.disturbances(), 0U);
        EXPECT_EQ(kept.pesters(), 0U);
        EXPECT_GT(kept.meals(), 0U);
        EXPECT_GT(kept.plays(), 0U);
    }

    TEST(PetTest, Play_TheShippedNumbersPunishPlayingWithoutPause)
    {
        Pet spent;
        for (Tick step = 0;
             step < 600 * antwika::companion::kTicksPerSecond
             && spent.state() != PetState::Perished;
             ++step)
        {
            spent.step();

            if (spent.state() == PetState::Awake)
            {
                spent.play();
            }
        }

        EXPECT_EQ(spent.state(), PetState::Perished);
    }
}
