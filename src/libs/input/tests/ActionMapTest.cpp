#include <gtest/gtest.h>

#include "antwika/input/ActionMap.hpp"
#include "antwika/input/ExtraModifiers.hpp"
#include "antwika/input/InputError.hpp"
#include "antwika/input/InputEvent.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/MouseButton.hpp"

using antwika::input::ActionMap;
using antwika::input::ExtraModifiers;
using antwika::input::InputError;
using antwika::input::InputState;
using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;

TEST(ActionMapTest, IsActive_ReportsFalseForAnActionNothingWasBoundTo)
{
    const ActionMap actions;
    const InputState state;

    EXPECT_FALSE(actions.isActive("move_left", state));
    EXPECT_FALSE(actions.wasTriggered("move_left", state));
}

TEST(ActionMapTest, IsActive_ReportsTrueWhileABoundKeyIsHeld)
{
    ActionMap actions;
    actions.bind("move_left", Key::A);

    InputState state;
    state.apply(KeyPressed{.key = Key::A});

    EXPECT_TRUE(actions.isActive("move_left", state));
}

TEST(ActionMapTest, IsActive_ReportsFalseWhileNothingBoundIsHeld)
{
    ActionMap actions;
    actions.bind("move_left", Key::A);

    InputState state;
    state.apply(KeyPressed{.key = Key::D});

    EXPECT_FALSE(actions.isActive("move_left", state));
}

TEST(ActionMapTest, IsActive_AnswersForEitherOfTwoBindings)
{
    ActionMap actions;
    actions.bind("move_left", Key::A);
    actions.bind("move_left", Key::ArrowLeft);

    InputState state;
    state.apply(KeyPressed{.key = Key::ArrowLeft});

    EXPECT_TRUE(actions.isActive("move_left", state));
}

TEST(ActionMapTest, IsActive_AnswersForABoundPointerButton)
{
    ActionMap actions;
    actions.bind("place", MouseButton::Left);

    InputState state;
    state.apply(PointerButtonPressed{.button = MouseButton::Left});

    EXPECT_TRUE(actions.isActive("place", state));
}

TEST(ActionMapTest, IsActive_ReportsFalseForADifferentPointerButton)
{
    ActionMap actions;
    actions.bind("place", MouseButton::Left);

    InputState state;
    state.apply(PointerButtonPressed{.button = MouseButton::Right});

    EXPECT_FALSE(actions.isActive("place", state));
}

TEST(ActionMapTest, IsActive_StaysTrueAfterTheTickThePressLandedOn)
{
    ActionMap actions;
    actions.bind("move_left", Key::A);

    InputState state;
    state.apply(KeyPressed{.key = Key::A});
    state.beginTick();

    EXPECT_TRUE(actions.isActive("move_left", state));
    EXPECT_FALSE(actions.wasTriggered("move_left", state));
}

TEST(ActionMapTest, WasTriggered_ReportsTrueOnlyOnTheTickOfThePress)
{
    ActionMap actions;
    actions.bind("jump", Key::Space);

    InputState state;
    state.apply(KeyPressed{.key = Key::Space});

    EXPECT_TRUE(actions.wasTriggered("jump", state));
}

TEST(ActionMapTest, WasTriggered_IgnoresAKeyThatIsMerelyHeld)
{
    ActionMap actions;
    actions.bind("jump", Key::Space);

    InputState state;
    state.apply(KeyPressed{.key = Key::Space, .repeat = true});

    EXPECT_TRUE(actions.isActive("jump", state));
    EXPECT_FALSE(actions.wasTriggered("jump", state));
}

TEST(ActionMapTest, WasTriggered_AnswersForABoundPointerButton)
{
    ActionMap actions;
    actions.bind("place", MouseButton::Right);

    InputState state;
    state.apply(PointerButtonPressed{.button = MouseButton::Right});

    EXPECT_TRUE(actions.wasTriggered("place", state));
}

TEST(ActionMapTest, WasTriggered_ReportsFalseForAnUnpressedPointerButton)
{
    ActionMap actions;
    actions.bind("place", MouseButton::Right);

    const InputState state;

    EXPECT_FALSE(actions.wasTriggered("place", state));
}

TEST(ActionMapTest, Bind_RequiresTheModifiersItAskedFor)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    InputState chordedState;
    chordedState.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});

    InputState bareState;
    bareState.apply(KeyPressed{.key = Key::S});

    EXPECT_TRUE(actions.isActive("save", chordedState));
    EXPECT_TRUE(actions.wasTriggered("save", chordedState));
    EXPECT_FALSE(actions.isActive("save", bareState));
    EXPECT_FALSE(actions.wasTriggered("save", bareState));
}

TEST(ActionMapTest, Bind_ChecksEveryModifierItAskedForIndependently)
{
    ActionMap actions;
    actions.bind(
        "everything",
        Key::S,
        KeyModifiers{
            .shift = true, .control = true, .alt = true, .super = true});

    InputState allState;
    allState.apply(
        KeyPressed{
            .key = Key::S,
            .modifiers = {
                .shift = true,
                .control = true,
                .alt = true,
                .super = true}});

    EXPECT_TRUE(actions.isActive("everything", allState));

    for (const auto missing : {0, 1, 2, 3})
    {
        KeyModifiers modifiers{
            .shift = true, .control = true, .alt = true, .super = true};

        if (missing == 0)
        {
            modifiers.shift = false;
        }
        else if (missing == 1)
        {
            modifiers.control = false;
        }
        else if (missing == 2)
        {
            modifiers.alt = false;
        }
        else
        {
            modifiers.super = false;
        }

        InputState state;
        state.apply(KeyPressed{.key = Key::S, .modifiers = modifiers});

        EXPECT_FALSE(actions.isActive("everything", state)) << missing;
    }
}

TEST(ActionMapTest, Bind_IgnoresModifiersHeldThatItDidNotAskFor)
{
    ActionMap actions;
    actions.bind("plain", Key::S);

    InputState state;
    state.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});

    EXPECT_TRUE(actions.isActive("plain", state));
}

TEST(ActionMapTest, WasTriggered_TakesTheModifiersThePressItselfCarried)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    InputState state;
    state.apply(KeyPressed{.key = Key::LeftControl, .modifiers = {}});
    state.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});
    state.apply(
        KeyReleased{.key = Key::LeftControl, .modifiers = {}});

    EXPECT_TRUE(actions.wasTriggered("save", state));

    EXPECT_FALSE(actions.isActive("save", state));
}

TEST(ActionMapTest, WasTriggered_IgnoresAModifierPressedAfterTheKey)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    InputState state;
    state.apply(KeyPressed{.key = Key::S, .modifiers = {}});
    state.apply(
        KeyPressed{
            .key = Key::LeftControl, .modifiers = {.control = true}});

    EXPECT_FALSE(actions.wasTriggered("save", state));
}

TEST(ActionMapTest, WasTriggered_TakesAButtonsOwnPressModifiers)
{
    ActionMap actions;
    actions.bind(
        "place", MouseButton::Left, KeyModifiers{.shift = true});

    InputState state;
    state.apply(
        PointerButtonPressed{
            .button = MouseButton::Left, .modifiers = {.shift = true}});
    state.apply(
        PointerButtonReleased{
            .button = MouseButton::Right, .modifiers = {}});

    EXPECT_TRUE(actions.wasTriggered("place", state));
}

TEST(ActionMapTest, WasTriggered_ForgetsAPressEdgeOnTheNextTick)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    InputState state;
    state.apply(
        KeyPressed{.key = Key::S, .modifiers = {.control = true}});

    ASSERT_TRUE(actions.wasTriggered("save", state));

    state.beginTick();

    EXPECT_FALSE(actions.wasTriggered("save", state));
}

TEST(ActionMapTest, Bind_KeepsActionsApart)
{
    ActionMap actions;
    actions.bind("left", Key::A);
    actions.bind("right", Key::D);

    InputState state;
    state.apply(KeyPressed{.key = Key::A});

    EXPECT_TRUE(actions.isActive("left", state));
    EXPECT_FALSE(actions.isActive("right", state));
}

TEST(ActionMapTest, Bind_ThrowsOnAnEmptyActionName)
{
    ActionMap actions;

    EXPECT_THROW(actions.bind("", Key::A), InputError);
}

TEST(ActionMapTest, Matches_AnswersForABindingAndTheModifiersHeld)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    EXPECT_TRUE(
        actions.matches("save", Key::S, KeyModifiers{.control = true}));
    EXPECT_FALSE(actions.matches("save", Key::S, KeyModifiers{}));
    EXPECT_FALSE(
        actions.matches("save", Key::D, KeyModifiers{.control = true}));
}

TEST(ActionMapTest, Matches_ReportsFalseForAnActionNothingWasBoundTo)
{
    const ActionMap actions;

    EXPECT_FALSE(actions.matches("save", Key::S, KeyModifiers{}));
}

TEST(ActionMapTest, Matches_AnswersForAMouseButtonToo)
{
    ActionMap actions;
    actions.bind("paint", MouseButton::Left);

    EXPECT_TRUE(actions.matches("paint", MouseButton::Left, KeyModifiers{}));
    EXPECT_FALSE(actions.matches("paint", MouseButton::Right, KeyModifiers{}));
}

TEST(ActionMapTest, Matches_TakesAModifierBesideTheOnesItAsksFor)
{
    ActionMap actions;
    actions.bind("save", Key::S, KeyModifiers{.control = true});

    EXPECT_TRUE(
        actions.matches(
            "save", Key::S, KeyModifiers{.shift = true, .control = true}));
}

TEST(ActionMapTest, Matches_RefusesAModifierBesideTheOnesItAsksFor)
{
    ActionMap actions;
    actions.bind(
        "save",
        Key::S,
        KeyModifiers{.control = true},
        ExtraModifiers::Refused);

    EXPECT_TRUE(
        actions.matches("save", Key::S, KeyModifiers{.control = true}));
    EXPECT_FALSE(
        actions.matches(
            "save", Key::S, KeyModifiers{.shift = true, .control = true}));
}

TEST(ActionMapTest, Matches_RefusesAnyModifierWhereItAsksForNone)
{
    ActionMap actions;
    actions.bind("brush", Key::B, KeyModifiers{}, ExtraModifiers::Refused);

    EXPECT_TRUE(actions.matches("brush", Key::B, KeyModifiers{}));
    EXPECT_FALSE(
        actions.matches("brush", Key::B, KeyModifiers{.control = true}));
    EXPECT_FALSE(
        actions.matches("brush", Key::B, KeyModifiers{.shift = true}));
}

TEST(ActionMapTest, IsActive_RefusesAModifierBesideTheOnesItAsksFor)
{
    ActionMap actions;
    actions.bind("brush", Key::B, KeyModifiers{}, ExtraModifiers::Refused);

    InputState state;
    state.apply(
        KeyPressed{.key = Key::B, .modifiers = KeyModifiers{.shift = true}});

    EXPECT_FALSE(actions.isActive("brush", state));
}

TEST(ActionMapTest, IsActive_TakesTheExactModifiersItAsksFor)
{
    ActionMap actions;
    actions.bind("brush", Key::B, KeyModifiers{}, ExtraModifiers::Refused);

    InputState state;
    state.apply(KeyPressed{.key = Key::B});

    EXPECT_TRUE(actions.isActive("brush", state));
}

TEST(ActionMapTest, WasTriggered_RefusesAModifierBesideTheOnesItAsksFor)
{
    ActionMap actions;
    actions.bind(
        "save",
        Key::S,
        KeyModifiers{.control = true},
        ExtraModifiers::Refused);

    InputState state;
    state.apply(
        KeyPressed{
            .key = Key::S,
            .modifiers = KeyModifiers{.shift = true, .control = true}});

    EXPECT_FALSE(actions.wasTriggered("save", state));
}

TEST(ActionMapTest, IsBound_SaysWhetherAnActionCarriesABinding)
{
    ActionMap actions;

    EXPECT_FALSE(actions.isBound("save"));

    actions.bind("save", Key::S);

    EXPECT_TRUE(actions.isBound("save"));
}

TEST(ActionMapTest, Rebind_LeavesTheActionOnItsNewBindingAlone)
{
    ActionMap actions;
    actions.bind("save", Key::S);
    actions.rebind("save", Key::F2);

    EXPECT_FALSE(actions.matches("save", Key::S, KeyModifiers{}));
    EXPECT_TRUE(actions.matches("save", Key::F2, KeyModifiers{}));
}

TEST(ActionMapTest, Unbind_LeavesTheActionCarryingNothing)
{
    ActionMap actions;
    actions.bind("save", Key::S);
    actions.unbind("save");

    EXPECT_FALSE(actions.isBound("save"));
    EXPECT_FALSE(actions.matches("save", Key::S, KeyModifiers{}));
}

TEST(ActionMapTest, Unbind_LeavesAnActionItNeverHeldAlone)
{
    ActionMap actions;
    actions.bind("save", Key::S);
    actions.unbind("load");

    EXPECT_TRUE(actions.isBound("save"));
}

TEST(ActionMapTest, Rebind_RefusesAnActionWithNoName)
{
    ActionMap actions;

    EXPECT_THROW(actions.rebind("", Key::S), InputError);
}
