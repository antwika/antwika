#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
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
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;

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

TEST(PetPreviewTest, Draw_WritesTheCompanionAtHome)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "companion",
             .title = "Antwika Companion",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                const antwika::companion::Translator translator{
                    antwika::i18n::kDefaultLocale};

                const PetScene scene(translator);
                scene.draw(renderer, kCanvas, awake());
            })
            .empty());
}
