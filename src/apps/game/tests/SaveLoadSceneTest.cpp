#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "WidgetPixel.hpp"

#include "TestTranslator.hpp"
#include "antwika/game/SaveLoadScene.hpp"
#include "antwika/game/SaveLoadState.hpp"

using antwika::game::tests::kTranslator;

namespace
{

    using antwika::game::SaveLoadScene;
    using antwika::game::SaveLoadState;
    using antwika::game::tests::widgetCentre;
    using antwika::gfx::Point;
    using antwika::gfx::Size;
    using antwika::gfx::mocks::MockRenderer;
    using antwika::ui::DrawList;
    using antwika::ui::DrawText;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;
    namespace saveWidgets = antwika::game::saveWidgets;

    constexpr Size kCanvas{.width = 1024, .height = 640};

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    [[nodiscard]] bool holds(
        const std::vector<std::string> &texts, const std::string &wanted)
    {
        for (const auto &text : texts)
        {
            if (text == wanted)
            {
                return true;
            }
        }

        return false;
    }

    // Where a widget is, is the layout's business.
    [[nodiscard]] Point pixelOn(
        const SaveLoadScene &scene,
        const SaveLoadState &state,
        WidgetId id)
    {
        const auto centre = widgetCentre(
            scene.describe(kCanvas, Pointer{}, Keyboard{}, state), id);

        return centre.value_or(Point{.x = -1, .y = -1});
    }

    TEST(SaveLoadSceneTest, Describe_ShowsThreeButtonsAndAPlaceholderEach)
    {
        const SaveLoadScene scene{kTranslator};
        const SaveLoadState state;

        const auto texts =
            textsOf(scene.describe(kCanvas, {}, {}, state).commands);

        EXPECT_TRUE(holds(texts, "SAVE / LOAD"));
        EXPECT_TRUE(holds(texts, "Save"));
        EXPECT_TRUE(holds(texts, "Load"));
        EXPECT_TRUE(holds(texts, "Back"));
        EXPECT_TRUE(holds(texts, "no saved games"));
        EXPECT_TRUE(holds(texts, "name a new save"));
    }

    TEST(SaveLoadSceneTest, Describe_ShowsWhatIsSelectedAndWhatIsTyped)
    {
        const SaveLoadScene scene{kTranslator};
        SaveLoadState state({"alpha", "beta"});
        state.select(1);
        state.setName("town", 4);
        state.setMessage("Saved town");

        const auto texts =
            textsOf(scene.describe(kCanvas, {}, {}, state).commands);

        EXPECT_TRUE(holds(texts, "beta"));
        EXPECT_TRUE(holds(texts, "town"));
        EXPECT_TRUE(holds(texts, "Saved town"));
    }

    // The open list is an overlay, so it takes no room from the card.
    // It is appended after every other command rather than among them.
    TEST(SaveLoadSceneTest, Describe_DrawsTheOpenListOverWhatWasThere)
    {
        const SaveLoadScene scene{kTranslator};
        SaveLoadState closed({"alpha", "beta"});
        SaveLoadState open({"alpha", "beta"});
        open.setListOpen(true);

        const auto shut = scene.describe(kCanvas, {}, {}, closed).commands;
        const auto dropped = scene.describe(kCanvas, {}, {}, open).commands;

        ASSERT_GT(dropped.size(), shut.size());
        EXPECT_TRUE(
            std::equal(shut.begin(), shut.end(), dropped.begin()));

        // The selected name in the box, and both options below it.
        const auto texts = textsOf(dropped);
        EXPECT_EQ(std::count(texts.begin(), texts.end(), "alpha"), 2);
    }

    TEST(SaveLoadSceneTest, Describe_ReachesEveryWidgetItDeclares)
    {
        const SaveLoadScene scene{kTranslator};
        SaveLoadState state({"alpha"});

        for (const auto id :
             {saveWidgets::kPicker,
              saveWidgets::kName,
              saveWidgets::kSave,
              saveWidgets::kLoad,
              saveWidgets::kBack})
        {
            EXPECT_NE(pixelOn(scene, state, id), (Point{.x = -1, .y = -1}));
        }
    }

    TEST(SaveLoadSceneTest, Draw_ClearsAndThenPaints)
    {
        const SaveLoadScene scene{kTranslator};
        const SaveLoadState state;
        ::testing::NiceMock<MockRenderer> renderer;

        ::testing::InSequence order;
        EXPECT_CALL(renderer, clear(::testing::_));
        EXPECT_CALL(renderer, drawRect(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber());

        scene.draw(
            renderer, scene.describe(kCanvas, {}, {}, state).commands);
    }

} // namespace
