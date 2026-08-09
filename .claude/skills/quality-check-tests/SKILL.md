---
name: quality-check-tests
description: Quality-check tests for the ways they fail to fail — comparing a computation to itself, asserting nothing, using a payload that cannot observe the property under test, or asserting a value the double supplied. Use when writing, reviewing, refactoring or converting tests in this repository, and before claiming a test covers a behaviour.
---

# Quality-checking tests

A test earns its place by failing when someone breaks the thing it names.

Before writing or accepting one, answer this out loud: **what edit would make
this
test go red?** Name the edit. If you cannot name a realistic one, the test is
documentation at best and a false signal at worst — it reports green while the
behaviour it claims to guard is unprotected.

Run the checklist at the bottom on every test you write or touch.

## Smell 1: comparing a computation to itself

The shape is `f() == f()`, usually wearing the word *determinism*.

```cpp
// Found in this repository, and it cannot fail.
TEST(EcsDeterminismTest, RunSimulation_IsIdenticalWhenRunTwice)
{
    const auto first = runSimulation(20);
    const auto second = runSimulation(20);

    EXPECT_EQ(first, second);
}
```

This can only catch nondeterminism **inside one process** — a read of
uninitialised memory, an iteration order derived from a heap address, a global
random seed. It cannot catch the failure that actually matters: an answer that
is
wrong but stable.

The decisive question is what regression the test is guarding against. Here it
is
someone changing a sparse set's `remove` from a stable shift to swap-and-pop —
the
standard optimisation, an obviously tempting edit, and one that reorders every
view and breaks replay determinism across versions. **The test above still
passes.** So does its neighbour that compares view order to itself, because a
fixed sequence of `std::vector` operations cannot differ between two calls.

Pin the answer instead of comparing a run to itself:

```cpp
TEST(ViewTest, Iterate_KeepsInsertionOrderAcrossARemoval)
{
    EXPECT_EQ(
        order,
        (std::vector<Entity>{created[0], created[2], created[3], extra}));
}
```

That fails the moment removal stops being stable, and it writes the ordering
contract down where a reader can find it.

Genuine cross-run determinism — same input, different process, machine or
build —
is not testable by calling a function twice in one process. It needs a recorded
expectation checked into the repository, which is what a replay or golden-file
test is for.

## Smell 2: a payload that cannot observe the property

A test can be order-sensitive in name and order-independent in fact.

```cpp
// Each entity's result depends only on itself, so this assertion holds
// under any iteration order at all: reversed, shuffled, anything.
world.set<Position>(
    entity,
    Position{position.x + velocity.dx, position.y + velocity.dy});
```

If the property is ordering, the payload has to record the order — append to a
shared log, or assert on the sequence the view yields. Ask whether shuffling the
input would change the result. If it would not, the test is not about ordering
however it is named.

## Smell 3: the assertion went missing

Converting a hand-written double to a mock is the usual way this happens: the
old
`EXPECT_EQ(spy.calls, 3)` is deleted and the replacing
`EXPECT_CALL(...).Times(3)`
is never added. The test compiles, passes, and checks nothing.

```cpp
// Before
CountingSink inner;
gate.handle(click());
EXPECT_EQ(inner.calls, 1U);

// After — and now nothing is asserted
NiceMock<MockTickEventSink> inner;
gate.handle(click());
```

When you rewrite assertions, do the removal and the replacement in one edit, and
read the finished test body. A test with no `EXPECT_`, `ASSERT_`,
`EXPECT_CALL` or
`FAIL` in it is a bug.

## Smell 4: the double supplies the answer

If the test's expectation is satisfied by behaviour the test itself wired into a
double, the production code is not being exercised.

```cpp
// The mock is told to produce exactly what the test then asserts.
ON_CALL(store, load()).WillByDefault(Return(kBitmap));
EXPECT_EQ(subject.reload(), kBitmap);
```

That passes if `reload()` returns its input unchanged and nothing else. Assert
on
something the code under test decided, not on a value handed to it.

## Smell 5: a tautology on a value the test just built

```cpp
const auto config = makeConfig();
EXPECT_EQ(config.width, makeConfig().width);
```

Two calls to the same builder, or a comparison of a field to the literal that
was
just assigned to it. Same family as smell 1, smaller.

## Naming a test double: Mock or Fake

Every type in a test that implements an interface is a test double, and it takes
one of two prefixes. There is no third.

**`Mock`** — built with `MOCK_METHOD`. The prefix records how the type is
made, so
it holds however the interface is used.

**`Fake`** — hand-written, with a real if simplified body.

`scripts/check_comment_style.py` fails a `Mock` with no `MOCK_METHOD` in it
and a
`Fake` with one, so the two cannot drift apart.

### Which one to reach for

The question is what the test claims, not what the interface looks like.

A claim about **the calls** takes a mock: that a method ran, how often, in what
order, with which arguments, or that it answered with a canned value or threw.

A claim about **state the double holds** takes a fake: the test writes through
it
and reads back, so the double needs a working implementation.

The round trip is the giveaway. If the test calls `save` and then asserts on
what
`load` gives back, no expectation can express that.

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

Reach for the mock first, because a mock is written once and a fake is written
once per behaviour. Every test writes its own expectations against one published
mock, which is why `MockLogger` serves a hundred and eighteen files, while this
tree once collected thirteen separate `ISystem` doubles next to an unused
`MockSystem`.

Two things send a double back to being a fake. A method returning something the
caller owns is one: gmock's default action hands back a null
`std::unique_ptr<ITexture>` unless every such method is stubbed in every test.
State that must stay consistent across calls is the other, which is why
`FakeClock`, `FakeSleeper` and `FakeDeck` are the most reused doubles here.

Before writing either kind, look for the published one that already exists.

### A distinction that was considered and rejected

Do not reintroduce the idea that an implementation of an interface the owning
library never implements is "test input" rather than a double, and so needs no
prefix. It was measured and thrown out.

`antwika::ecs` ships no `ISystem`, which makes it look like a bare extension
point. But **27 production systems implement it** in `game`, `life`,
`task_worker`, `simulation` and `ecs_commons`. A test system is a simplified
stand-in for those, which is exactly what a fake is. Whether the interface's own
library also ships one is a fact about layering, not about the double's role.
The same holds for `IMigration` with eight production implementations and
`IWordReader` with two.

Three further reasons it was rejected:

- The standard taxonomy — dummy, fake, stub, spy, mock — classifies a double by
  what the substitute does, never by who owns the interface.
- The prefix exists to warn a reader that a type is test-only and simplified.
  `RecordingJob` in a published header reads like production code;
  `FakeRecordingJob` does not. That value is the same either way.
- The test is unstable and barely checkable. Its answer depends on whether some
  unrelated file exists elsewhere in the tree, so shipping one built-in system
  would force eight renames for a reason invisible at any declaration.
  `MOCK_METHOD` is a local, permanent fact by comparison.

## The checklist

Before accepting any test:

1. Name the edit that turns it red. If none, delete it or pin a real
expectation.
2. Does it compare a computation to itself? Pin the value instead.
3. If it names a property such as ordering, could a payload that ignores that
   property still pass? Then it is not testing the property.
4. Does the body contain an assertion at all, and does the assertion reference
   something the code under test produced?
5. Would the assertion still hold if the production code were replaced by
   `return input;`? Then it is asserting the double, not the code.

## Where this came from

An `EcsDeterminismTest` in this repository held two tests of the first kind.
Both
were green, both had been green for their whole life, and neither could catch
the
one regression the file existed to prevent. The smell was found by reading the
implementation and asking what change would break the test — not by reading the
test, which looked reasonable.
