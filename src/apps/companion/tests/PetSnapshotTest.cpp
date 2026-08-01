#include <gtest/gtest.h>

#include "antwika/companion/Pet.hpp"
#include "antwika/companion/PetSnapshot.hpp"
#include "antwika/companion/Saying.hpp"

using antwika::companion::Pet;
using antwika::companion::PetConfig;
using antwika::companion::PetSnapshot;
using antwika::companion::PetState;
using antwika::companion::Saying;
using antwika::companion::snapshotOf;

namespace
{
    constexpr PetConfig kQuick{
        .dayTicks = 4,
        .nightTicks = 6,
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .sayingTicks = 2,
        .chatterPeriodTicks = 2,
        .hungerMax = 4,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .feedJoy = 1,
        .disturbCost = 2,
        .happinessMax = 6,
        .happinessStart = 4};

    TEST(PetSnapshotTest, ASnapshotIsWhatTheCompanionLooksLike)
    {
        Pet pet(kQuick);
        pet.step();
        pet.step();

        const PetSnapshot snapshot = snapshotOf(pet);

        EXPECT_EQ(snapshot.state, PetState::Awake);
        EXPECT_FALSE(snapshot.night);
        EXPECT_TRUE(snapshot.hungry);
        EXPECT_FALSE(snapshot.disturbed);
        EXPECT_EQ(snapshot.hunger, 2U);
        EXPECT_EQ(snapshot.hungerMax, kQuick.hungerMax);
        EXPECT_EQ(snapshot.happiness, kQuick.happinessStart);
        EXPECT_EQ(snapshot.happinessMax, kQuick.happinessMax);
        EXPECT_EQ(snapshot.ticks, 2U);

        // The second tick is a chatter tick, so it has found a line.
        // Which line is Pet's business, and the snapshot carries it.
        EXPECT_NE(snapshot.saying, Saying::None);
        EXPECT_EQ(snapshot.saying, pet.saying());
    }

    TEST(PetSnapshotTest, ASleepingCompanionCarriesItsNightIntoThePicture)
    {
        Pet pet(kQuick);
        for (int step = 0; step < 4; ++step)
        {
            pet.step();
        }
        pet.tap();

        const PetSnapshot snapshot = snapshotOf(pet);

        EXPECT_EQ(snapshot.state, PetState::Asleep);
        EXPECT_TRUE(snapshot.night);
        EXPECT_TRUE(snapshot.disturbed);
    }

    // Two snapshots of one companion are the same value.
    // That is what lets a whole frame's state be asserted at once.
    TEST(PetSnapshotTest, TheSameCompanionSnapshotsToTheSameValue)
    {
        Pet pet(kQuick);
        pet.step();

        EXPECT_EQ(snapshotOf(pet), snapshotOf(pet));
    }

    // Equality is on the whole value.
    // So a picture differing in any one thing is a different picture.
    // A scene test comparing two frames rests on that.
    // It has to hold of every field, not the ones checked first.
    TEST(PetSnapshotTest, ADifferenceInAnySingleFieldIsADifferentValue)
    {
        const PetSnapshot base{
            .state = PetState::Awake,
            .night = false,
            .hungry = false,
            .disturbed = false,
            .saying = Saying::None,
            .hunger = 1,
            .hungerMax = 4,
            .happiness = 3,
            .happinessMax = 6,
            .ticks = 2};

        PetSnapshot other = base;
        other.state = PetState::Asleep;
        EXPECT_NE(base, other);

        other = base;
        other.night = true;
        EXPECT_NE(base, other);

        other = base;
        other.hungry = true;
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
        other.happiness = 4;
        EXPECT_NE(base, other);

        other = base;
        other.happinessMax = 7;
        EXPECT_NE(base, other);

        other = base;
        other.ticks = 3;
        EXPECT_NE(base, other);
    }
} // namespace
