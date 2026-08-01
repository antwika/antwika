#include <gtest/gtest.h>

#include "antwika/companion/LifeStage.hpp"

using antwika::companion::CareRecord;
using antwika::companion::formFor;
using antwika::companion::PetForm;

namespace
{
    TEST(LifeStageTest, Form_ACompanionNeverWrongedIsBright)
    {
        EXPECT_EQ(
            formFor(CareRecord{.meals = 1, .plays = 1}),
            PetForm::Bright);
    }

    // Untested rather than exemplary.
    // Nothing at all is not the same as having been done well by.
    TEST(LifeStageTest, Form_ACompanionNothingHappenedToIsPlain)
    {
        EXPECT_EQ(formFor(CareRecord{}), PetForm::Plain);
    }

    TEST(LifeStageTest, Form_TwiceAsMuchGoodAsBadIsStillPlain)
    {
        EXPECT_EQ(
            formFor(CareRecord{.meals = 2, .plays = 2, .pesters = 2}),
            PetForm::Plain);
    }

    TEST(LifeStageTest, Form_AnythingLessThanThatIsScruffy)
    {
        EXPECT_EQ(
            formFor(CareRecord{.meals = 3, .pesters = 2}),
            PetForm::Scruffy);
    }

    // A collapse counts for three of anything else.
    // It is the one violation that takes something back.
    TEST(LifeStageTest, Form_OneCollapseWeighsAsMuchAsThreeSlights)
    {
        EXPECT_EQ(
            formFor(CareRecord{.meals = 6, .collapses = 1}),
            PetForm::Plain);
        EXPECT_EQ(
            formFor(CareRecord{.meals = 5, .collapses = 1}),
            PetForm::Scruffy);
        EXPECT_EQ(
            formFor(CareRecord{.meals = 6, .disturbances = 3}),
            PetForm::Plain);
    }
} // namespace
