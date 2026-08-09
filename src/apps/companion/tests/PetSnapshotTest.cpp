#include <gtest/gtest.h>

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetSnapshot.hpp"
#include "antwika/companion/Saying.hpp"

using antwika::companion::DayMood;
using antwika::companion::LifeStage;
using antwika::companion::Lineage;
using antwika::companion::LineageMemory;
using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetForm;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::companion::Saying;
using antwika::companion::snapshotOf;

namespace
{
    constexpr PetConfig kQuick{
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .funDecayPeriodTicks = 1000,
        .fretPeriodTicks = 1000,
        .recoverPeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .sayingTicks = 2,
        .chatterPeriodTicks = 2,
        .drainHappyTicks = 1000,
        .drainContentTicks = 1000,
        .drainLowTicks = 1000,
        .drainMiserableTicks = 1000,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 4,
        .funStart = 4,
        .playEnergy = 4,
        .energyBase = 8,
        .collapsePenalty = 4,
        .tiredPercent = 50,
        .happinessMax = 6,
        .happinessStart = 4};

    TEST(PetSnapshotTest, SnapshotOf_ShowsWhatTheCompanionLooksLike)
    {
        Pet pet(kQuick);
        const Lineage lineage;
        pet.step();
        pet.step();

        const PetSnapshot snapshot = snapshotOf(pet, lineage);

        EXPECT_EQ(snapshot.state, PetState::Awake);
        EXPECT_FALSE(snapshot.asleep);
        EXPECT_TRUE(snapshot.hungry);
        EXPECT_FALSE(snapshot.bored);
        EXPECT_FALSE(snapshot.tired);
        EXPECT_FALSE(snapshot.disturbed);
        EXPECT_EQ(snapshot.hunger, 2U);
        EXPECT_EQ(snapshot.hungerMax, kQuick.hungerMax);
        EXPECT_EQ(snapshot.fun, kQuick.funStart);
        EXPECT_EQ(snapshot.funMax, kQuick.funMax);
        EXPECT_EQ(snapshot.happiness, kQuick.happinessStart);
        EXPECT_EQ(snapshot.happinessMax, kQuick.happinessMax);
        EXPECT_EQ(snapshot.energy, kQuick.energyBase);
        EXPECT_EQ(snapshot.energyCeiling, kQuick.energyBase);
        EXPECT_EQ(snapshot.ticks, 2U);
        EXPECT_EQ(snapshot.day, 0U);
        EXPECT_EQ(snapshot.mood, DayMood::Ordinary);
        EXPECT_EQ(snapshot.stage, LifeStage::Egg);
        EXPECT_EQ(snapshot.form, PetForm::Plain);

        EXPECT_NE(snapshot.saying, Saying::None);
        EXPECT_EQ(snapshot.saying, pet.saying());
    }

    TEST(PetSnapshotTest, SnapshotOf_CarriesTheNightWhenAsleep)
    {
        Pet pet(kQuick);
        const Lineage lineage;
        pet.play();
        ASSERT_TRUE(pet.tired());
        pet.putToBed();

        const PetSnapshot snapshot = snapshotOf(pet, lineage);

        EXPECT_EQ(snapshot.state, PetState::Asleep);
        EXPECT_TRUE(snapshot.asleep);
        EXPECT_TRUE(snapshot.tired);
        EXPECT_FALSE(snapshot.disturbed);
    }

    TEST(PetSnapshotTest, SnapshotOf_CarriesAShorterCeilingWhenCollapsed)
    {
        Pet pet(kQuick);
        const Lineage lineage;
        pet.play();
        pet.play();
        ASSERT_EQ(pet.collapses(), 1U);

        const PetSnapshot snapshot = snapshotOf(pet, lineage);

        EXPECT_EQ(snapshot.energy, 0U);
        EXPECT_EQ(
            snapshot.energyCeiling,
            kQuick.energyBase - kQuick.collapsePenalty);
    }

    TEST(PetSnapshotTest, SnapshotOf_CarriesTheRecordBehindIt)
    {
        const Pet pet(kQuick);
        Lineage lineage;
        lineage.record(400);
        lineage.advance();

        const PetSnapshot snapshot = snapshotOf(pet, lineage);

        EXPECT_EQ(snapshot.lineage.generation, 2U);
        EXPECT_EQ(snapshot.lineage.bestTicks, 400U);
    }

    TEST(PetSnapshotTest, OperatorEquals_MatchesTwoSnapshotsOfOneCompanion)
    {
        Pet pet(kQuick);
        Pet twin(kQuick);
        const Lineage lineage;
        pet.step();
        twin.step();

        EXPECT_EQ(snapshotOf(pet, lineage), snapshotOf(twin, lineage));
    }

    TEST(PetSnapshotTest, OperatorEquals_SeparatesADifferenceInAnyField)
    {
        const PetSnapshot base{
            .state = PetState::Awake,
            .asleep = false,
            .hungry = false,
            .bored = false,
            .tired = false,
            .disturbed = false,
            .saying = Saying::None,
            .hunger = 1,
            .hungerMax = 4,
            .fun = 2,
            .funMax = 4,
            .happiness = 3,
            .happinessMax = 6,
            .energy = 5,
            .energyCeiling = 8,
            .ticks = 2,
            .day = 1,
            .mood = DayMood::Ordinary,
            .stage = LifeStage::Egg,
            .form = PetForm::Plain,
            .lineage = LineageMemory{}};

        PetSnapshot other = base;
        other.state = PetState::Asleep;
        EXPECT_NE(base, other);

        other = base;
        other.asleep = true;
        EXPECT_NE(base, other);

        other = base;
        other.hungry = true;
        EXPECT_NE(base, other);

        other = base;
        other.bored = true;
        EXPECT_NE(base, other);

        other = base;
        other.tired = true;
        EXPECT_NE(base, other);

        other = base;
        other.disturbed = true;
        EXPECT_NE(base, other);

        other = base;
        other.saying = Saying::Hello;
        EXPECT_NE(base, other);

        other = base;
        other.hunger = 2;
        EXPECT_NE(base, other);

        other = base;
        other.hungerMax = 5;
        EXPECT_NE(base, other);

        other = base;
        other.fun = 3;
        EXPECT_NE(base, other);

        other = base;
        other.funMax = 5;
        EXPECT_NE(base, other);

        other = base;
        other.happiness = 4;
        EXPECT_NE(base, other);

        other = base;
        other.happinessMax = 7;
        EXPECT_NE(base, other);

        other = base;
        other.energy = 6;
        EXPECT_NE(base, other);

        other = base;
        other.energyCeiling = 9;
        EXPECT_NE(base, other);

        other = base;
        other.ticks = 3;
        EXPECT_NE(base, other);

        other = base;
        other.day = 2;
        EXPECT_NE(base, other);

        other = base;
        other.mood = DayMood::Heavy;
        EXPECT_NE(base, other);

        other = base;
        other.stage = LifeStage::Teen;
        EXPECT_NE(base, other);

        other = base;
        other.form = PetForm::Bright;
        EXPECT_NE(base, other);

        other = base;
        other.lineage.generation = 4;
        EXPECT_NE(base, other);
    }
}
