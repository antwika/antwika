# Comment and documentation style

This guide is built one decision at a time.
Every rule below was approved explicitly; nothing here is a default that crept in.

Lines in this file hold one sentence each, because `scripts/check_one_sentence_per_line.py` enforces that over `docs/**/*.md` and over the comments in `src/` and `backends/`.

## When a block is required

A documentation block appears where, and only where, there is a contract to state.

A contract means a precondition, a postcondition, a thrown exception, an ownership transfer, a unit, or a rule about lifetime or reentrancy.
A declaration with none of those carries no block, however public it is.

```cpp
/**
 * @brief Advances the scene by one tick.
 *
 * @param dt Seconds since the last tick; must be greater than zero.
 * @return The tick number just completed.
 * @throws std::invalid_argument If dt is not greater than zero.
 *
 * Requires: begin() has been called and reset() has not.
 * Ensures:  frame() is non-null until the next reset().
 */
std::uint64_t step(float dt);

int width() const;
bool empty() const;
```

`width()` and `empty()` are not undocumented by oversight.
A block on either would restate the signature, and adding one is a violation rather than an improvement.

## What a block may say: the contract, and nothing else

A block states what the caller must guarantee going in, and what holds coming out.

It does not explain rationale.
It does not describe mechanism.
It does not record history, rejected alternatives, or the bug that prompted the change.

## Format: Doxygen

When an API is documented, it is documented with Doxygen block syntax and explicit tags.

Use `/** ... */` with `@brief`, `@param`, `@return` and `@throws`.
Do not use plain `//` prose for an API contract, and do not use the `///` or `//!` variants.

The tags are required even when the signature makes them guessable, because the value of a structured block is that it is uniform.
A half-tagged block is worse than either extreme, since a reader cannot tell whether a missing `@throws` means "throws nothing" or "nobody wrote it down".

There is no `Doxyfile` in the repository and no documentation-generation step in CI.
The syntax is chosen for its uniformity and tooling support, not because pages are generated from it today.

## A complete example

There is no Doxygen block anywhere in the tree today, so there is nothing to copy from.

This is the specimen.
A block that departs from its layout should have a reason.

```cpp
namespace antwika::game
{

    class GridScene
    {
    public:
        /**
         * @brief Advances the scene by one tick.
         *
         * @param dt Seconds since the last tick; must be greater than
         *           zero.
         * @return The tick number just completed.
         * @throws std::invalid_argument If dt is not greater than zero.
         *
         * Requires: begin() has been called and reset() has not.
         * Ensures:  frame() is non-null until the next reset().
         */
        std::uint64_t step(float dt);

        int width() const;
    };
}
```

The layout is fixed: `@brief` first, a blank comment line, then the parameter and result tags, then a blank comment line, then any `Requires:` and `Ensures:` lines.
Continuation lines align under the start of the text they continue.
Lines wrap at the same eighty columns the rest of the code does, which `scripts/check_line_length.py` enforces.

`width()` in that example is deliberate.
It has no contract, so it carries no block, and adding one would be a violation.

## Every claim in a block needs a test

A block makes claims, and each claim is backed by a test that exercises it.

Every `@throws` needs a test that provokes the throw.
Every `Requires:` needs a test that violates the precondition and shows what happens.
Every `Ensures:` needs a test that asserts the postcondition holds.

A claim with no test behind it is a comment wearing a tag, and it drifts the moment the code moves.

```cpp
/**
 * @brief Advances the scene by one tick.
 *
 * @throws std::invalid_argument If dt is not greater than zero.
 *
 * Ensures: frame() is non-null until the next reset().
 */
std::uint64_t step(float dt);

TEST(GridSceneTest, Step_RejectsANonPositiveDt)
{
    EXPECT_THROW(scene.step(0.0F), std::invalid_argument);
}

TEST(GridSceneTest, Step_LeavesAFrameBehind)
{
    scene.step(1.0F);

    EXPECT_NE(scene.frame(), nullptr);
}
```

Deleting a claim is always allowed and needs no test.
It is adding one that costs something, which is the right way round.

The `@throws` half of this is close to free, because the coverage gate already demands every branch be taken, and an unprovoked throw path fails coverage on its own.
The `Requires:` and `Ensures:` halves are not, and they are checked in review.

This rule does not reach a `@param` that lies about its units.
Nothing mechanical will catch that, and pretending otherwise would be its own kind of false claim.

## Rationale lives in a test name

A decision worth remembering is pinned by a test whose name states the constraint.

The name is the explanation, and unlike a comment it fails when someone undoes the decision.
A reason that cannot be expressed as a failing test is a reason that was not load-bearing.

```cpp
TEST(GridSceneTest, Sweep_RespectsClearOrdering)
{
    ...
}
```

A sentence such as "we tried a queue here but the ordering was wrong" belongs nowhere in the source.
The test above is the durable record of it, and it is checked on every run.

## A fixture name ends in Test

The first argument of a `TEST` macro names what is under test and ends in `Test`.

```cpp
TEST(GridSceneTest, Sweep_RespectsClearOrdering)
TEST(AStarTest, Find_ReturnsTheShortestPath)
TEST_P(PointerToggleSinkTest, Toggle_FlipsTheCell)

// Rejected.
TEST(GridSceneSuite, Sweep_RespectsClearOrdering)
TEST(TestGridScene, Sweep_RespectsClearOrdering)
```

The suffix is all the rule asks for.

The prefix is not required to name a type, because many fixtures have no type to name: `AStarTest` covers a free function, `ConsoleIntegrationTest` covers a path through several modules, and `EcsDeterminismTest` covers a property rather than a thing.
Demanding a type from those would mean inventing one, and a fixture named after an invented type is worse than a fixture named after what it tests.

A fixture name is held to the terseness rule like any other name, not to the seventy-five characters test names get.

A typed suite is a fixture and takes the suffix too.

`TYPED_TEST_P` names a class that several backends are instantiated against, which makes it a fixture reused rather than a second kind of thing.
The suffix goes on the suite, not on the prefix an instantiation supplies, because the prefix names the type under test rather than the suite.

```cpp
template <typename Traits>
class GfxBackendConformanceTest : public ::testing::Test

TYPED_TEST_P(GfxBackendConformanceTest, Close_IsIdempotent)

INSTANTIATE_TYPED_TEST_SUITE_P(
    Raylib,
    GfxBackendConformanceTest,
    RaylibBackendTraits);

// Rejected.
class GfxBackendConformance : public ::testing::Test
INSTANTIATE_TYPED_TEST_SUITE_P(RaylibTest, GfxBackendConformance, Traits);
```

This rule is true of every fixture in the tree, and `scripts/check_comment_style.py` fails on one that breaks it.

It was not true of the typed suites when it was written, because the checker did not read `TYPED_TEST_P` at all and eight suites had grown names ending in `Conformance` or `Contract` instead.
They were renamed in one pass, which is what a rule that is mechanical rather than a matter of judgement costs.

## Test names are Method_DoesX

A test name has two parts joined by an underscore: the thing under test, then what it does.

The rule binds every gtest macro that declares a test, `TYPED_TEST_P` among them.

```cpp
TEST(GridSceneTest, Sweep_RespectsClearOrdering)
TEST(GridSceneTest, Step_RejectsANonPositiveDt)
TEST(WaveTest, Release_PlansNothingBeyondTheLastWave)
TYPED_TEST_P(NetworkBackendConformanceTest, OpenHost_HoldsNoPeers)
```

The first part names a method, not a scenario.
The second says what happens, in the present tense, without `should` or `will`.

Where the thing under test is not a named member function, the first part still names the C++ entity rather than inventing a concept for it.

- An operator is named as itself: `OperatorEquals`, `OperatorCompare`, `OperatorPlus`, `OperatorIncrement`.
- A constructor is `Ctor`, spelled that way and no other.
- A free function is its own name, as `TypedTextFor` or `ParseVoiceChain`.

This was got wrong once and cost 395 renames to put right.
The first pass through the tree used `Equality`, `Construction`, `Ordering`, `Arithmetic` and `Throw`, which are readable but are concepts nobody can grep for, and which the rule above does not actually permit.
`Ctor` in particular replaced four spellings of the same idea that had accumulated separately.

A prefix naming a type rather than a function, as `KeyPressed_DefaultsToAFreshPress` does, is allowed.
The type is what the test is about, and there is no function to name instead.

A test that asserts a property over every value still names the call that reaches them.

```cpp
// Rejected.
TYPED_TEST_P(MessageSetCompletenessTest,
             EveryLocaleTranslatesWhatItDoesNotShare)

// Required instead.
TYPED_TEST_P(MessageSetCompletenessTest,
             CatalogueFor_TranslatesWhatALocaleDoesNotShare)
```

Looping is how the test is written rather than what it is about, so the loop does not earn a name of its own.

A condition is folded into the second part rather than given a clause of its own, because `Method_WhenY_DoesX` spends length on grammar rather than on meaning.

A test name may run to seventy-five characters, and one character more is rejected.

Seventy-five is not an arbitrary number: it is the longest identifier that fits on its own line inside the eighty-column limit, once the five spaces that indent it are taken off.
A C++ identifier cannot be split across lines, so the column limit and the name limit are the same rule counted twice.
Test names are held to the eighty-column limit like every other line, with no exemption.

```cpp
TEST(HoldemPotsTest,
     DistributePots_GivesAnOddChipToTheFirstSeatLeftOfTheButton)
```

This rule is true of every test in the tree, and `scripts/check_comment_style.py` fails on a name that breaks it.

It was not true when it was written.
2486 names were single prose phrases such as `AWavesSizeIsTheSumOfItsEntries`, and they were renamed a module at a time under the migration terms below.

A later pass caught eighteen more in the typed suites, which the checker had never read.
Those were sentences about a document or a host, and each method half came off the body: `TextThatIsNotJsonIsRefused` became `LoadFileOrDefaults_RefusesTextThatIsNotJson` because that is the call the test makes.

The rename could not be automated outright, because the method half of a name is not recoverable from the old one.
`AWavesSizeIsTheSumOfItsEntries` became `Size_IsTheSumOfItsEntries` only once somebody had read the test and confirmed that `size()` is what it exercises.

Two things made the work tractable and are worth knowing if a rule like this is ever added again.
The prefix can be proposed by matching each test body against the module's API, which is right often enough to be worth checking rather than typing.
A proposed name must never be truncated to fit the length limit, because a cut name can silently collide with another in the same fixture.

## Names stay terse, outside of tests

A name carries meaning, but it is not a sentence.

A function or constant that needs forty characters is a signal that the abstraction is wrong.
Split the concept rather than lengthen its name.

```cpp
// Rejected.
int wrapColumnOntoTorus(int x, int width);
constexpr std::uint32_t kFirstSeedWithASpawnOrderingTie = 41;

// Required instead.
int wrapColumn(int x, int width);
constexpr std::uint32_t kTieSeed = 41;
```

Test names are the deliberate exception, and the rule above gives them seventy-five characters.

They are the only place in the codebase where a name is the whole record of a decision, so they are allowed to spend length saying what that decision was.
Everything else can be explained by the code around it.

## Function bodies carry no comments

There are no comments below a signature.

A body that needs a comment to be readable is a body that needs a named function.
Extracting one turns the explanation into something the compiler checks and the next reader can call.

```cpp
// Rejected.
void tick()
{
    // Wrap the column so the map behaves as a torus.
    x = (x % w + w) % w;
}

// Required instead.
void tick()
{
    x = wrapColumn(x, w);
}
```

This rule has no exception for a long body, a subtle body, or a body written under time pressure.
Those are the cases it exists for.

The one exception is the three phase markers inside a test body, given below.
They are not an explanation of any code, which is what this rule is about.

## Tests follow the same rules, but for the phase markers

A test file is production code for the purposes of this guide, with one exemption.

A test body may carry `// Arrange`, `// Act` and `// Assert`, and nothing else.
No fixture carries a banner, and every other comment is banned in a test exactly as it is anywhere else.

```cpp
TEST(GridSceneTest, Step_LeavesAFrameBehind)
{
    // Arrange
    GridScene scene;

    // Act
    scene.step(1.0F);

    // Assert
    EXPECT_NE(scene.frame(), nullptr);
}
```

The three spellings are exact and the list is closed.
`// Arrange:`, `// arrange` and `// Setup` are comments, and are therefore banned.
A marker sits on its own line; one trailing a statement is a comment about that statement rather than a heading over a phase.

The markers are optional.
A test that reads without them does not gain anything by adding them.

They are exempt because they name the phase of a test rather than explaining its code.
That is the same reason a `#` heading is not prose: it says where the reader is, not what to think.
Nothing else in a test body earns that, which is why the list is three words long and closed.

A magic constant in a test is named rather than explained.

```cpp
// Rejected.
TEST(WaveTest, Release_PlansNothingBeyondTheLastWave)
{
    // 41 is the first seed that produces a tie in the spawn ordering.
    Wave wave{Settings{.seed = 41}};
    ...
}

// Required instead.
TEST(WaveTest, Release_PlansNothingBeyondTheLastWave)
{
    constexpr std::uint32_t kTieSeed = 41;

    Wave wave{Settings{.seed = kTieSeed}};
    ...
}
```

## A test double is named for how it is built

There are two kinds of test double and the prefix says which one a reader is looking at.

`Mock` means a gmock double, declared with `MOCK_METHOD`.
`Fake` means a hand-written implementation with a real, if simplified, body.
No other prefix exists: there is no `Stub`, no `Spy` and no `Dummy` in the tree, and adding one is a change to this document.

```cpp
class MockLogger : public ILogger
{
public:
    MOCK_METHOD(void, log,
                (Level level, std::string_view message),
                (noexcept, override));
};

class FakeRng : public IRng
{
public:
    std::uint32_t next() noexcept override { return value; }

    std::uint32_t value = 0;
};
```

The prefix is worth spending because the two behave differently under failure.
An unmet `MOCK_METHOD` expectation fails on its own at destruction, and a hand-written fake fails only where a test asserts on what it recorded.

## Choosing between a mock and a fake

The question is what the test claims, not what the interface looks like.

A test that claims something about **the calls** takes a mock: that a method ran, how often, in what order, with which arguments, or that it answered with a canned value or threw.
A test that claims something about **state the double holds** takes a fake: it writes through the double and reads back, so the double needs a real, if simplified, implementation.

The round trip is the giveaway.
If the test calls `save` and then asserts on what `load` gives back, no expectation can express that and the double needs to hold the bitmap.

```cpp
// A mock: the claim is that three systems ran in order.
NiceMock<MockSystem> a, b, c;
{
    InSequence order;
    EXPECT_CALL(a, update(_, _));
    EXPECT_CALL(b, update(_, _));
    EXPECT_CALL(c, update(_, _));
}

// A fake: the claim is that what was saved comes back.
class FakeAtlasStore final : public IAtlasStore
{
public:
    void save(const Bitmap &image) override { written = image; }

    std::optional<Bitmap> load() override { return written; }

private:
    std::optional<Bitmap> written{};
};
```

Reach for the mock first, because a mock is written once and a fake is written once per behaviour.

Every test writes its own expectations against one published mock, so `MockLogger` serves a hundred and eighteen files.
A fake's behaviour is baked into it, so a second test wanting a different behaviour writes a second fake, and the tree collected thirteen separate `ISystem` doubles while `MockSystem` sat published and unused by them.

Two things send a double back to being a fake.

A method returning something the caller owns is one: gmock's default action returns a default-constructed value, so a `NiceMock` hands back a null `std::unique_ptr<ITexture>` unless every such method is stubbed in every test.
State that has to stay consistent across calls is the other, which is why `FakeClock`, `FakeSleeper` and `FakeDeck` are the most reused doubles in the tree.

A published double is written per interface, not per test.

Before writing either kind, look for the one that already exists.

The prefix binds every double, not only one another module links.

A type in a test that implements an interface is a double whether it is published or declared beside the test that uses it, and the prefix is what tells a reader which of the two kinds it is reading.
The name keeps whatever distinguishes it, because a file may hold several doubles of one interface: `SchedulerTest.cpp` had three implementations of `IJob`, so `FakeJob` alone would not have named them.

```cpp
class FakeRecordingJob final : public IJob
class FakeReschedulingJob final : public IJob
class FakeDestructionTrackingJob final : public IJob

// Rejected.
class RecordingJob final : public IJob
class LaggingDevice final : public IDevice
```

`scripts/check_comment_style.py` fails a `Mock` type with no `MOCK_METHOD` in it, and a `Fake` type with one.

This rule is true of every double in the tree, and the checker fails one that breaks it.

It was not true when it was written: every double in the published trees carried a prefix, and 62 declared inside a single test file did not.
They were renamed a module at a time, and a library double moved into its module's `fakes` tree as it was renamed, because the placement rule reaches any `Fake` under `src/libs/`.

Three things that migration found are worth knowing.

A double that captures a constant its test declares is parameterised rather than moved with it: `FakeCountMigration` takes the versions it migrates between, where the local class read a `kToyVersion` beside it.
A type that exists only to hold what a double recorded travels with the double, because it is that double's result rather than the test's data: `Sounded` moved into the sequencer's `fakes` tree along with the sink that fills it.
Unifying two copies of one double is part of the work rather than a bonus, and `FakeReschedulingJob` absorbed a constructor argument that only one of its two copies had.

A double that operates on a type the test defines for its own sake is a template on that type, which publishes no test data: `FakeSetPositionSystem` takes the component it writes, and `FakeSnapshotStore` the error it throws.

One conflating idea was measured and rejected while this ran.
An implementation of an interface the owning library never implements looks like test input rather than a double, but 27 production systems implement `ISystem` and eight implement `IMigration`, so a test one stands in for something real after all.

## A namespace-scope constant takes a k prefix

A `constexpr` declared at namespace scope is named `k` followed by what it is.

```cpp
namespace antwika::game
{
    constexpr std::uint32_t kMaxTicks = 600;
    constexpr Size kAtlasSlotSize{.width = 8, .height = 8};

    // Rejected.
    constexpr std::uint32_t maxTicks = 600;
    constexpr std::uint32_t MAX_TICKS = 600;
}
```

The prefix is Google's convention rather than anything the standard says, and it is kept for one reason: at namespace scope the use site can be in a different file from the declaration, and `kMaxTicks` says what it is without going to look.

`MAX_TICKS` is the older answer and a worse one in C++, because upper case is macro territory and a system header may define anything.
A `k` name cannot collide with a macro, since no one writes macros that way.

Inside a function body the prefix is not required, and not forbidden either.

There the declaration is a few lines above the use, so the name has nothing left to carry.
The tree is split roughly two to one in favour of keeping it there, and neither spelling is worth a rule.

```cpp
TEST(WalkingTest, NextFacing_TurnsAtACorner)
{
    constexpr Neighbours corner{.east = true, .south = true};
    constexpr Neighbours kCorridor{.north = true, .south = true};
}
```

A static data member is outside the rule too, because its name is always reached through its type.

## A type nothing derives from is final

Every class and struct is `final` unless something in the tree derives from it.

```cpp
struct KeyPressed final
{
    Key key;
};

class ILogger
{
public:
    virtual void log(Level level, std::string_view message) = 0;
};

// Rejected.
struct KeyPressed
{
    Key key;
};
```

`final` is the declaration saying no more is coming, and it is worth writing because the reader is otherwise left to search for a derived class that does not exist.

Three kinds of type are not `final`, and none of them is an exception a writer chooses.

A base is not `final`, which is what the rule already says.
A gtest fixture is not, because `TEST_F` derives from it at macro expansion and the compiler rejects the result.
A gmock double reached through `NiceMock`, `StrictMock` or `NaggyMock` is not, because those wrappers derive from it too.

```cpp
class GridSceneTest : public ::testing::Test
{
};

class MockLogger : public ILogger
{
};

NiceMock<MockLogger> logger;
```

The build is what found the third case.

Marking every leaf type `final` compiled everywhere except thirteen mocks, and `gmock-nice-strict.h` reported each as deriving from a `final` base.
A rule that reaches this far into a template library is one to land with a build rather than an argument.

## An abstract type takes an I prefix

A type that declares a pure virtual function is named `I` followed by what it is.

```cpp
class ILogger
{
public:
    virtual void log(Level level, std::string_view message) = 0;
};

template <typename ErrorT>
class IJsonSnapshotStore : public ISnapshotStore
{
protected:
    virtual nlohmann::json takeState(const std::string &path) = 0;
};

// Rejected.
class Logger
{
public:
    virtual void log(Level level, std::string_view message) = 0;
};
```

The prefix says the type cannot be instantiated, which is what a reader needs before deciding whether to hold one by value.

It says nothing about whether the type is a pure seam.
`IJsonSnapshotStore` carries a constructor, a data member and two `final` overrides, and it takes the prefix on the same terms as `ILogger`, which carries nothing at all.
Drawing the line at "no state and no implementation" would mean reading the whole body to know how to name it, and the answer would change the day somebody adds a default.

The rule reaches a type that declares a pure virtual, which is what a checker can see.

A type made abstract only by inheriting a pure virtual it does not override is outside it, because deciding that needs the whole hierarchy rather than one body.
No type in the tree is abstract that way today.

## A header whose whole content is one type is named for it

Where a header declares one type and nothing else, the file name and the type name are the same word.

```cpp
// Rejected: antwika/game/SnapshotStore.hpp
namespace antwika::game
{
    class GameSnapshotStore final
    {
    };
}

// Required instead: antwika/game/GameSnapshotStore.hpp
namespace antwika::game
{
    class GameSnapshotStore final
    {
    };
}
```

The rule reaches only that shape, because it is the only one where the file has a single answer to what it holds.

A header of free functions is named for what it provides, and the type it happens to declare along the way does not take the name off it.
`antwika/io/File.hpp` declares `enum class Content` and the read and write functions that take it, and renaming the file to `Content.hpp` would name it after its least important member.
A header of several types is named for what they have in common, as `Events.hpp` is.

A test helper header is named for what it provides, and never leads with `Test`.

A helper beside a test is a header like any other, and the same question answers its name: what does a reader come here for?
`Test` at the front answers a different question, the one the directory already answered, and it sorts every helper in a module under the same letter.

```cpp
// Rejected.
apps/game/tests/TestTranslator.hpp   // provides kTranslator, kLanguages
libs/i18n/tests/TestMessages.hpp     // provides struct TestMessages
apps/game/tests/WidgetPixel.hpp      // provides widgetCentre()

// Required instead.
apps/game/tests/Translators.hpp
libs/i18n/tests/SampleMessages.hpp   // provides struct SampleMessages
apps/game/tests/WidgetCentre.hpp     // provides widgetCentre()
```

The leading `Test` is the half a checker can see, and it fails a header whose name starts with it.
Whether the rest of the name says what the header provides is settled in review, like the other rules about meaning.

This is the same objection the fixture rule makes to `TestGridScene`, applied to a file rather than a type.

The implementation file follows the header.

An include block leads with the file's own header, and the checker finds that header by matching the stem.
Renaming `SnapshotStore.hpp` and leaving `SnapshotStore.cpp` behind makes the first include of that file stop being its own, which the include-order rule catches immediately.

## A library publishes its doubles, an app keeps them

A double a library owns lives in `tests/mocks/include/` or `tests/fakes/include/`, under the module's own namespace directory.

That layout is not decoration.
It is an exported CMake target, so another module can link `antwika::log::tests::mocks` and use the double rather than writing a second one.
`scripts/check_unused_test_doubles.py` reads exactly those two path fragments, and a double outside them is a double nothing checks.

```cpp
src/libs/log/tests/mocks/include/antwika/log/mocks/MockLogger.hpp
src/libs/time/tests/fakes/include/antwika/time/fakes/FakeClock.hpp
```

An application publishes nothing, so its doubles stay beside its tests.

There is no target for another module to link, because no other module may depend on an app.
A double shared by two of an app's test files gets a header next to them, and one used by a single file is declared inside that file.

```cpp
src/apps/game/tests/FakeMenuCommands.hpp

// src/apps/poker/tests/BootstrapTest.cpp, above the tests that use it.
namespace
{
    class FakeTexture final : public ITexture
    {
    };
}
```

The split follows who can reach the double, not how large it is.

A helper that is not a double follows the same split, in a `support` tree.

`mocks`, `fakes`, `conformance` and `support` are the four kinds a module may publish, and `support` holds what is none of the other three: an assertion, a script, a fixture.
A helper only one module's tests use needs no target and sits loose beside them.

```cpp
// Published, because four apps' tests link it.
src/libs/ui/tests/support/include/antwika/ui/support/WidgetCentre.hpp
src/libs/ui/tests/support/include/antwika/ui/support/DrawBounds.hpp

// Loose, because one module reads it.
src/apps/task_worker/tests/DemoScript.hpp
src/apps/life/tests/BlinkerScript.hpp
src/libs/ui/tests/LinesOf.hpp
```

A helper earns a target when a second module needs it, and not before.

## Placement: no file or namespace banners

A file opens directly with `#pragma once` and its includes.

There is no banner comment, no module description block, and no license header.
A closing brace does not carry a `} // namespace antwika::game` comment.

```cpp
#pragma once

#include <cstdint>

namespace antwika::game
{
    enum class MessageId : std::uint16_t
    {
        ToolbarZoomIn,
    };
}
```

What a module is for belongs in the names of the things it exports, not in a banner that every reader scrolls past.
Namespace-closing comments restate a brace that an editor already matches.

## No unfinished-work markers

`TODO`, `FIXME`, `HACK` and `XXX` do not appear in committed code.

Unfinished work lives in the issue tracker, where it can be prioritised, assigned and closed.
A marker in a source file is invisible to everyone deciding what to work on next, and it survives long after the reason for it has gone.

## Tool-directed markers: a closed list

Some comments are read by a tool rather than a person, and those are exempt from the rules above.

The list of permitted markers is closed.
A marker that is not named here is a comment, and is therefore banned.

- `GCOVR_EXCL_LINE`
- `GCOVR_EXCL_START`
- `GCOVR_EXCL_STOP`

```cpp
    } // GCOVR_EXCL_LINE
```

These three are load-bearing: `scripts/coverage.sh` feeds the build to `gcovr`, and `scripts/check_full_coverage.py` fails the build at anything below 100% lines, functions and branches.
Deleting one does not remove a comment, it removes an exclusion and breaks the gate.

Adding a fourth marker to this list is a change to this document, made deliberately, and not something a tool adoption does silently.
`NOLINT`, `clang-format off` and `IWYU pragma` are not on the list and do not appear in the tree today.

The test phase markers are not on this list and never join it.
They are read by a person rather than a tool, and they are permitted only inside a test body, so they are a separate exemption with its own section and its own scope.

## Every language, not just C++

The rules bind Python, CMake, shell, the CI workflows and the dev container images exactly as they bind C++.

A `#` comment is a comment, and none appear in `scripts/`, `cmake/`, `CMakeLists.txt` or `.github/workflows/`.
This includes shell comments written inside a `run: |` block, which are comments in a script that happens to live in a YAML file.

Three things that look like comments are not, and all of them stay:

- A shebang such as `#!/usr/bin/env python3` on the first line of a script.
- A Dockerfile parser directive, `# syntax=` or `# escape=`, which changes how the file is read.
- A `#` that falls inside a string, such as the `#include` lines in the C++ source that `cmake/AntwikaEmbedBinary.cmake` writes, or the `${GITHUB_REF#refs/tags/v}` expansion in a workflow.

The second case is why the checker parses rather than greps.
Deleting those `#include` lines produces a generated file that does not compile.

## A Python docstring is a comment

A string opening a module, a class or a function is prose in the position a comment would sit in, and it is banned exactly as a `#` comment is.

```python
# Rejected.
def disc(east: float, south: float, radius: float) -> bool:
    """Whether a point is inside a disc centred in the cell."""
    dx = east - 0.5

# Required instead.
def disc(east: float, south: float, radius: float) -> bool:
    dx = east - 0.5
```

The syntax is the only thing that separates the two.
A docstring explains mechanism to a person reading the file, which is what the function-body rule already refuses in C++, and the signature above it says what the name and the types say.

Nothing in this repository reads one.
There is no Sphinx, no pydoc and no documentation step in CI, and no script is imported by another, so a docstring here has no reader a comment would not have.

`argparse.ArgumentParser()` is therefore constructed bare.

Passing `description=__doc__` promises `--help` a description that a module without a docstring can only ever supply as `None`, which every script in `scripts/` did.
A parser that wants a description states it as a string literal, which ships and is behaviour rather than commentary.

## Every Python function is annotated

Every parameter and every return carries a type, with no exception for a test.

```python
def find_violations(root: Path) -> list[Violation]:
def run_main(root: Path, *extra: str) -> int:
def it_flags_a_source_list_out_of_order() -> None:

# Rejected.
def find_violations(root):
def it_flags_a_source_list_out_of_order():
```

`self` and `cls` are not annotated, because their type is the class the method is in.

`-> None` on a test that takes nothing does say less than the other annotations do.
It is required anyway, because the alternative is a rule with a shape to remember, and a rule about which functions are exempt is one more thing to get wrong than a rule about all of them.

This rule cost a rename, which is worth knowing before adding another rule that lengthens a line.

`-> None` is eleven characters, and it pushed a Python test name past the eighty-column limit exactly as the seventy-five character test-name rule did in C++.
The name was shortened rather than the line wrapped, because a `def` line that wraps before its colon reads worse than a shorter name.

## Include order

Includes appear in five groups, each separated from the next by a blank line.

1. The file's own header, in quotes.
2. Third-party headers, in angle brackets.
3. Standard library headers, in angle brackets.
4. Project headers from another module, in angle brackets.
5. Project headers from this module, in quotes.

```cpp
#include "antwika/game/GridScene.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

#include <antwika/gfx/Color.hpp>

#include "antwika/game/Direction.hpp"
```

A group with nothing in it is simply absent; there are no placeholders.

The own header comes first so that a header which fails to compile alone fails here rather than somewhere downstream.
The angle-bracket and quote split says whether a header belongs to this module, which is a fact worth being able to read off the include list.

Only the leading include block is ordered.

The block ends at the first line that is not an include, a blank line, or `#pragma once`.
An include below that point is outside the rule, because something in between has changed what it means.
`src/libs/font/src/StbTrueType.cpp` is the case that forced this: `<stb_truetype.h>` has to follow the `#define STB_TRUETYPE_IMPLEMENTATION` that decides what the header expands to, and moving it up would compile a translation unit with no implementation in it.

Includes inside a preprocessor conditional are not ordered either.

The branches of an `#ifdef` are alternatives rather than a sequence, so there is no order to hold them to.
`backends/sockets/src/SocketApi.cpp` picks `<ws2tcpip.h>` or the POSIX socket headers this way.

The checker reports only the first out-of-order include in a file, because the rest are usually the same mistake repeated.

## A source list is alphabetical

Every list of source files in a `CMakeLists.txt` is in alphabetical order.

```cmake
SOURCES
    src/SessionGatedSystem.cpp
    src/SessionStore.cpp
    src/SpawnSystem.cpp
    src/StandingBuildings.cpp
    src/StateDump.cpp

# Rejected.
SOURCES
    src/SessionStore.cpp
    src/StateDump.cpp
    src/StandingBuildings.cpp
    src/SpawnSystem.cpp
```

The order has no effect on the build, which is exactly why it needs a rule.

Nothing pushes an entry back into place, so a list drifts one rename at a time until finding a file in it means reading all hundred lines.
Two renames in the session that wrote this rule landed in the wrong place, and both were invisible until the whole list was sorted.

A run of entries at one indentation is one list, and a file may hold several.

The checker treats a line that is not a source path, or a change of indentation, as the end of a list.
It reports the first entry out of place in each, because the rest are usually that one entry's consequence.

## Data members follow the function rule

A data member carries a block when it has a contract, and not otherwise.

An invariant, a unit, or a rule about when it may be read or written is a contract.
A member that is simply a value the type holds is not, and it stays bare.

```cpp
struct Node
{
    /**
     * @brief The layout box, in the parent's coordinates.
     *
     * Requires: set before layout() runs.
     */
    Rect bounds;

    int childCount;
    Axis axis;
};
```

`childCount` and `axis` are values the type holds, and naming them is the whole of what needs saying.
`bounds` has an ordering rule that the type does not enforce for itself, so it says so.

A member whose contract is really about the type as a whole belongs in a test instead.

## A type carries no block

A class or struct never carries a documentation block, however large or subtle it is.

A contract belongs to a function or to a data member, and both of those have their own rule above.
What is left over is the whole-type invariant, and that is pinned by a test rather than written down.

```cpp
// Rejected.
/**
 * @brief A node in the layout tree.
 *
 * Invariant: bounds always fits inside the parent's bounds.
 */
struct Node
{
    Rect bounds;
};

// Required instead.
struct Node
{
    Rect bounds;
};

TEST(NodeTest, Layout_KeepsAChildInsideItsParent)
{
    ...
}
```

This is the same answer the two neighbouring rules already give.
The data-member rule sends a member contract that is really about the type as a whole to a test, and the enumerator rule sends a renumbering rule the same way.
Naming it here closes the gap rather than adding a policy.

No class or struct under `src/` or `backends/` carries a block, and the checker fails one that does.

## An enum is scoped and sized

Every enum is an `enum class` and states its underlying type.

```cpp
enum class MobKind : std::uint8_t
{
    Grunt = 0,
    Runner = 1,
};

// Rejected.
enum class MobKind
{
    Grunt = 0,
};

enum MobKind : std::uint8_t
{
    Grunt = 0,
};
```

The underlying type is not decoration, and the reason is not size.

Casting an integer into an enum with a fixed underlying type is defined for every value that type can hold, which is what makes the range check after the cast a real check.
Without one the underlying type is `int`, and casting in a value no enumerator names is undefined.
`src/apps/tower_defence/src/LevelTile.cpp` casts a byte back into a `Tile`, and `src/libs/pathfinding/include/antwika/pathfinding/NodeId.hpp` does the same, so this is the shape the tree actually uses.

Size follows from it: an unsized `enum class` is four bytes and a `std::uint8_t` one is a single byte, which matters wherever an enum sits in an array or a packed struct.

The rule binds an enum that has no cast into it today, because the rule is about the next one.

## Enumerators stay bare

An enumerator carries no block.

It has no precondition, no postcondition and nothing to throw, so by the rule above it never earns one.
The enumerator's name is what says which case it is.

```cpp
enum class MobKind : std::uint8_t
{
    Grunt = 0,
    Runner = 1,
    Brute = 2,
    Shielded = 3,
};
```

This holds even when the numeric values are persisted or sent over a wire.
A rule about renumbering is a property of the whole type rather than of any one case, so it belongs to a test that pins the values, in keeping with the rest of this guide.

## Message strings are not documentation

A log line and an exception's `what()` are output, not commentary.

No rule in this guide constrains them.
They are behaviour: a user reads them, a test asserts on them, and they change when the behaviour changes.

```cpp
throw std::invalid_argument{"dt must be greater than zero"};

log.warn("No sound device; using the null backend.");
```

This is said out loud because a message string is the obvious place to smuggle a banned comment, and the rule is that it is not one.
A string is exempt because it ships, not because prose inside quotation marks stops being prose.

The one string that does not ship is a failure message streamed into an assertion, and the next section rules on it.

## A streamed failure message carries values, not prose

An `EXPECT_` or `ASSERT_` macro prints its own operands, so a stream after it is adding to a message that already says something.

What it may add is a value naming which case failed.
What it may not add is a string literal, because the only thing prose can say there is what the operands or the test name have said already.

```cpp
// Rejected.
EXPECT_EQ(scene.level(), below) << "grew a tick early";
EXPECT_NE(name, "unknown") << "at index " << index;

// Required instead.
EXPECT_EQ(scene.level(), below) << index;
EXPECT_NE(name, "unknown") << index;
```

The first rejected line sits inside a loop over every housing level, and `"grew a tick early"` is the test's own name in other words.
What a reader needs on failure is which level, and that is `index`.

Two values are separated by a character literal, because `' '` cannot be widened into a sentence and `" "` can.

```cpp
EXPECT_LE(eased.numerator(), eased.denominator())
    << index << ' ' << step;
```

`FAIL()` and `ADD_FAILURE()` are exempt and may carry prose.

They take no operands, so a bare `FAIL();` reports a line number and nothing else.
Almost every use sits in a `try` whose `catch` inspects the error, which is a shape `EXPECT_THROW` cannot express.

```cpp
try
{
    static_cast<void>(toString(beyond));
    FAIL() << "expected an InputError";
}
catch (const InputError &error)
{
    EXPECT_NE(
        std::string(error.what()).find(
            std::to_string(kMouseButtonCount)),
        std::string::npos);
}
```

`SCOPED_TRACE` is exempt on the same grounds and may carry prose.

It takes no operands either, so its string is the whole of what it says rather than a gloss on something gtest has already printed.
A trace also heads a scope rather than an assertion, and a heading that is only a number tells a reader nothing about which loop it came from.

```cpp
SCOPED_TRACE("seed " + std::to_string(seed));
```

`GTEST_SKIP` is exempt, and with it the list closes at four.

A skip reason names the capability the backend under test does not have, and it is the one thing that survives into the ctest report, where the guard above it cannot follow.
A run that says a test was skipped and not why is indistinguishable from a backend quietly losing a feature.

```cpp
if (!this->linksUp())
{
    GTEST_SKIP() << "this backend does not link two hosts";
}
```

The exempt macros are `FAIL`, `ADD_FAILURE`, `SCOPED_TRACE` and `GTEST_SKIP`, and a macro not named here is not exempt.
This is a closed list in the same sense as the tool markers: adding a fifth is a change to this document, not something a new gtest facility does on its way in.

This rule is true of every assertion in the tree, and the checker fails on one that breaks it.

It was not true when it was written: 75 assertions across 35 files streamed a string literal, 21 of them prose alone and 54 a label in front of a value.
The migration ran a module at a time and closed the same day, because most of it was dropping a label from a stream that already carried the value worth having.

Thirteen sites lost their stream outright rather than gaining a value.
Those assertions were not in a loop, so the operands gtest already prints were the whole of what identified the failure, and the prose was restating the test name.

## The build directory is out of scope

Nothing under `build/`, `build-coverage/` or any other `build-*` directory is covered by this guide.

Those directories hold output, not source.
No rule in this document applies to a file there, no checker glob reaches one, and a comment found in one is not a violation.

This is stated explicitly because the exclusion is otherwise invisible.
`cmake/AntwikaEmbedBinary.cmake` writes `BuiltInFontBytes.cpp` into `build/`, and that generated file opens with a do-not-edit banner that would be a violation anywhere else in the tree.
It stays, and it needs no special-case rule of its own, because the directory it lands in is out of scope.

Deleting a `build-*` directory and rebuilding must always be safe.
That is the test of whether something belongs there.

## Agent configuration is out of scope

Nothing under `.claude/` is covered by this guide.

Those files configure the tools that work on the repository rather than describing the repository itself, in the same way the `build-*` directories hold output rather than source.
No checker glob reaches them.

`.claude/skills/revise-style-guide/SKILL.md` is the procedure by which this document changes: how a rule is proposed, approved, written down and enforced.
It is deliberately not a rule about the code, which is why it sits outside the two documents named below.

## Enforcement

`scripts/check_comment_style.py` gates the mechanically checkable rules on every build.

For C++ it reports a violation for an unfinished-work marker, for a `//` comment that is neither a permitted tool marker nor a phase marker on its own line in a test body, for a `/* ... */` comment that is not a Doxygen block, for a Doxygen block without `@brief`, and for any other comment inside a function body.
It also reports a Doxygen block attached to a class or struct, a fixture name that does not end in `Test`, a string literal streamed into an `EXPECT_` or `ASSERT_` macro, a `Mock` type with no `MOCK_METHOD` in its body, a `Fake` type with one, a library double declared outside the two published trees, a type declaring a pure virtual without the `I` prefix, a namespace-scope `constexpr` without the `k` prefix, an enum that is unscoped or states no underlying type, a type nothing derives from that is not `final`, a header whose name leads with `Test`, and a header whose whole content is one type it is not named for.
For the other languages it reports any comment at all, for Python it also reports a docstring and a function that is not fully annotated, for CMake it also reports a source list out of alphabetical order, and it reports a markdown file outside `README.md`, `CHANGELOG.md` and `docs/`.

Every rule it applies to a `TEST` reaches `TYPED_TEST_P` as well: the suite suffix, the name grammar, the seventy-five character limit, and the phase markers a test body may carry.

The rules it cannot check are the ones about meaning: whether a block states a contract rather than a rationale, whether a name is terse, whether a decision was pinned by a test, and whether every claim in a block has a test behind it.
Those are settled in review, and this document is what review appeals to.

The coverage gate does some of that work by accident.
An `@throws` nobody provokes leaves an uncovered branch, and `scripts/check_full_coverage.py` fails on it without knowing why.

## There is no escape hatch

No marker suppresses a rule for a line, a block or a file.

`check-comment-style: disable` and anything resembling it does not exist and will not be added.
The two closed lists, the tool markers and the test phase markers, are the only exemptions, and the `build-*` directories are the only excluded paths.

Neither list is an escape hatch, because neither can be pointed at arbitrary text.
A suppression marker exempts whatever a writer puts next to it; these six words exempt only themselves.

A rule that can be waived per line is a rule that gets waived, and a gate reporting zero violations while suppressions accumulate underneath is worse than no gate.
When a rule is wrong, this document changes, and the tree follows.

## Neither document states a count of the tree

This rule binds `README.md` as well as this file, which is both of the documents the repository writes by hand.

A rule is written without counting how many places in the tree already follow it, and the same holds for a sentence describing the project.

The test is whether the number would change when somebody adds a file.
If it would, it does not go in, and a phrase naming the thing takes its place.
A number that is accurate today is not thereby allowed, because accuracy is the state it decays from rather than a property it keeps.

A count is true on the commit that measured it and drifts on the next, and nothing ever re-measures it.
Two of the counts this document used to carry were wrong by the time anyone read them.
One said the longest test name was fifty-eight characters, while a name sat exactly on the seventy-five character limit.
The other was stale inside the hour that wrote it.

```
Rejected.
This rule is true of every fixture in the tree today: 569 of 569.
There are 786 class and struct declarations in the headers.
A C++ project built around deterministic simulation: 33 libraries and 13 applications.

Required instead.
This rule is true of every fixture in the tree, and the checker fails on one that breaks it.
No class or struct in the tree carries a block.
A C++ project built around deterministic simulation: the libraries under `src/libs/` and the applications under `src/apps/`.
```

The qualitative form is the stronger claim rather than the vaguer one.

`569 of 569` is a measurement nobody repeats, so it decays into a number that was once true.
`every fixture in the tree` is what `scripts/check_comment_style.py` re-proves on every run, so it is either true or the build is red.
A claim about the tree with no gate behind it is one this document should not be making at all.

Three kinds of number stay, because none of them is a measurement of the tree as it stands.

A number that is the rule stays: the seventy-five character test name, the eighty-column line, the seventy-two character commit subject, the five include groups, the hundred percent the coverage gate demands.
A number about a finished migration stays, because the past tense cannot rot: 2486 test names renamed, and 395 renames spent undoing a mistake, are what they were on the day.
A version of something the project did not write stays, because it identifies an artefact rather than counting one: Roboto Mono 3.001 and the SIL Open Font License 1.1 must be exact, and a licence version above all.

Both documents satisfy this rule.

`README.md` was the last holdout: its opening sentence counted the libraries and the applications, and both figures were wrong before anyone noticed.
It now names the two directories instead, which is the phrase the example above gives.
The figures were accurate again by the time they were removed, and that is not a reason to have kept them.

Nothing gates this rule, so it stays settled in review like the rules about meaning.

## Amending this guide

A rule changes here first, and the tree catches up afterwards.

The alternative, changing a rule and every file it touches in one commit, makes a small correction expensive enough that nobody proposes one.
So an amendment lands on its own, and the migration follows in as many commits as it takes.

During that window the rule under migration is named in `MIGRATING_RULES` in `scripts/check_comment_style.py`.

A rule on that list is counted and reported on every run, but does not fail the build.
Every other rule keeps failing exactly as before, which is the point: opening a migration for one rule must not quietly stop enforcing the others.

```
75 site(s) awaiting migration:
  string literal streamed into an assertion: 75
```

The entry comes off the list in the commit that finishes the migration.
The list is empty today, and all four migrations it has carried, include order, test-name grammar, streamed prose and the double prefix, were closed rather than left standing.

This is the single concession to the previous section, and it is shaped so it cannot become one.
Migration is per rule rather than per line, the list lives in one place a reader can grep, and the count is printed on every run so a stalled migration is loud rather than invisible.

A whole-tree `--warn-only` flag also exists, which downgrades everything at once.
It is for the rare amendment that touches every rule, and it is not the mechanism a single rule change should reach for.

## Commit messages

A commit message is the one place prose is expected rather than tolerated.

The subject line follows the conventional-commit form the release tooling parses, since `.releaserc` derives the version from it.
Use the imperative mood, and keep the subject under seventy-two characters.

```
fix(game): wrap the column rather than the index

A single pass let a mob move into a tile that was cleared later in
the same tick. The sweep now runs in two passes.

Pinned by GridSceneTest.Sweep_RespectsClearOrdering.
```

The body says what changed and what it affects.
It is not the home for rationale either, because rationale belongs in a test name, and a commit body is unreachable from the code it describes without a `git blame` first.

Naming the test that pins the change, as above, is the useful thing a body can do.
It gives a reader following `git log` the one link back into the tree that will not rot.

## What prose the repository keeps

Two documents about the project, one generated changelog, and no others.

`README.md` says what the project is and how to build and test it.
`docs/STYLE_GUIDE.md` is this file, which exists because a gate appeals to it.

`CHANGELOG.md` is the exception, and it is an exception because nobody writes it.
`semantic-release` derives it from the commit log on every release, so the rule it would otherwise break is one the commit messages already carry.
It sits at the root because that is where the tooling writes it and where a reader looks for it.
It stays out of `docs/` so that the prose gates do not run over generated text, because a rule a commit message can break is a rule that breaks a release.

There is no architecture note, no wiki and no blog in the tree.

A licence a bundled asset ships under is not one of the documents, and it stays.

`assets/fonts/LICENSE.txt` is the SIL Open Font License that `RobotoMono-Regular.ttf` requires to travel with it.
That is a legal obligation rather than a document about the project, and it is copied byte for byte rather than written.
Where a licence needs explaining, the explanation goes in `README.md`, which is one of the documents.

Files under `.claude/` are not counted here, for the reason given above.

This rule was broken once and is now checked.

`assets/fonts/README.md` grew to forty-seven lines about layering, embedding and asset bundling, and it linked three times to a `wiki/` directory that has never existed in this tree.
That is the whole argument for the rule: a document nothing gates drifts until it cites things that are not there.
`scripts/check_comment_style.py` now fails on any markdown outside `README.md`, `CHANGELOG.md` and `docs/`.
