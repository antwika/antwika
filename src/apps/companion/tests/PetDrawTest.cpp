#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/i18n/Locale.hpp>

#include "antwika/companion/Messages.hpp"
#include "antwika/companion/PetScene.hpp"
#include "antwika/companion/PetSnapshot.hpp"

namespace
{
    using antwika::companion::DayMood;
    using antwika::companion::LifeStage;
    using antwika::companion::PetForm;
    using antwika::companion::PetScene;
    using antwika::companion::PetSnapshot;
    using antwika::companion::PetState;
    using antwika::companion::Saying;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using ::testing::_;
    using ::testing::AtLeast;
    using ::testing::NiceMock;

    constexpr Size kCanvas{.width = 512, .height = 512};

    [[nodiscard]] PetSnapshot awake()
    {
        return PetSnapshot{
            .state = PetState::Awake,
            .hungry = true,
            .saying = Saying::FeedMe,
            .hunger = 6,
            .hungerMax = 8,
            .fun = 7,
            .funMax = 10,
            .happiness = 6,
            .happinessMax = 10,
            .energy = 30,
            .energyCeiling = 60,
            .ticks = 900,
            .day = 3,
            .mood = DayMood::Ordinary,
            .stage = LifeStage::Adult,
            .form = PetForm::Plain};
    }
}

TEST(PetDrawTest, Draw_DrawsTheCompanionAtHome)
{
    NiceMock<MockRenderer> renderer;

    EXPECT_CALL(renderer, drawRect(_, _)).Times(AtLeast(1));

    const antwika::companion::Translator translator{
        antwika::i18n::kDefaultLocale};

    const PetScene scene(translator);

    scene.draw(renderer, kCanvas, awake());
}
