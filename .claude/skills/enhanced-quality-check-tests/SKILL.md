---
name: enhanced-quality-check-tests
description: Quality-check tests against four questions — can this test fail, is its premise established, is it about one thing, and does anything else already cover it. Extends quality-check-tests with the premise, focus, redundancy and isolation checks, and ships detectors under tools/. Use when writing, reviewing or pruning tests, when asking whether the suite could be smaller, and before claiming a test covers a behaviour.
---

# Quality-checking tests, enhanced

`quality-check-tests` asks one question: **what edit would make this test
go red?** That question is necessary and it is not sufficient. A test can
answer it perfectly and still be worthless, because it duplicates its
neighbour, or because it is about four things at once, or because its
assertion only holds when an earlier line happened to produce something.

This skill supersedes `quality-check-tests` and carries its five smells
forward unchanged. Read that skill for the worked examples behind smells
1 to 5; the checklist below is the operative artifact and is complete on
its own.

Four questions now, in order. A test must pass all four.

1. **Can it fail?** Name the edit that turns it red.
2. **Is its premise established?** Would it also pass if the production
   code produced nothing at all?
3. **Is it about one thing?** Does it fail for exactly the reason its
   name gives?
4. **Does anything else already cover it?** If another test dies to every
   fault this one dies to, one of them is furniture.

## Question 1: can it fail?

The five original smells, unchanged:

1. A computation compared to itself — `f(x) == f(x)`, usually wearing the
   word *determinism*. It cannot catch an answer that is wrong but
   stable. Pin a recorded value instead.
2. A payload that cannot observe the property. Ask whether shuffling the
   input would change the result; if it would not, the test is not about
   ordering however it is named.
3. The assertion went missing. A body with no `EXPECT_`, `ASSERT_`,
   `EXPECT_CALL` or `FAIL` is a bug.
4. The double supplies the answer. Assert on something the code decided,
   not on a value the test handed it.
5. A tautology on a value the test just built.

Two more, both found by measurement on this repository:

**Smell 6: a comparison blind to identity.** `EXPECT_EQ(base, base)`
looks like a harmless reflexivity check. It is not. Replace the type's
`operator==` with `return this == &other;` and it still passes — and so
does every `EXPECT_NE(base, mutatedCopy)` beside it, because those are
distinct objects. Forty-six tests in this tree, all named
`OperatorEquals_ComparesEveryField` or similar, were blind to an identity
comparison. Compare against an independently built equal value:

```cpp
const auto twin = base;
EXPECT_EQ(base, twin);
```

**Smell 7: an assertion that never runs.** If every `EXPECT_` in a body
sits inside a `for` or an `if`, an empty collection means the body never
executes and the test passes having checked nothing.

```cpp
// Green when the road-preview pass is deleted outright: no blit matches
// the filter, the loop body never runs, and nothing is asserted.
for (const auto &blit : renderer.blits)
{
    if (blit.tint == kPreview)
    {
        EXPECT_EQ(blit.cell, expected);
    }
}
```

Count the matches and assert the count, then the loop has teeth.

**Smell 8: no expectation that can fail.** A mock whose every
`EXPECT_CALL` carries `Times(AnyNumber())` asserts nothing at all.

```cpp
// Green when ui::paint is deleted from the scene entirely.
EXPECT_CALL(renderer, drawRect(_, _)).Times(AnyNumber());
```

`Times(AnyNumber())` beside a stricter expectation is the opposite — it
is the idiom for silencing calls you do not care about so the strict one
can be sequenced. Thirty uses on this tree are all of that legitimate
kind, which is why the detector fires only when *no* expectation in the
body has a concrete cardinality.

## Question 2: is its premise established?

This was the single most common defect on this tree, and it is the one
reviewers miss most, because the test reads as if it says something.

**Vacuous when empty.** Two collections are compared and both would be
empty if the production code did nothing.

```cpp
// A Playback that never renders a sample passes this.
EXPECT_EQ(plain.rendered.samples, echoed.rendered.samples);
```

**A sentinel that starts at the passing value.** An index defaults to 0,
then an ordering is asserted against it. Delete the whole pass that was
meant to set it and the assertion still holds.

```cpp
std::size_t lastScrim = 0;              // never proven to have been set
...
EXPECT_GT(walkerAt, lastScrim);         // holds when no scrim was drawn
```

**A fixture where the interesting term is zero.** A 45px canvas over four
11px cells leaves one spare pixel, and half of one is zero — so the
centring term the test is named for cannot be observed. Choose 46px.

The fix is almost always one line, placed before the comparison:

```cpp
ASSERT_FALSE(rendered.samples.empty());
ASSERT_GT(scrims, 0U);
```

A test that pins at least one absolute value has an established premise.
A test whose every assertion is a relation between two things it
computed does not.

## Question 3: is it about one thing?

`Method_Behaviour` naming makes the claimed subject legible, so this is
mostly a reading task with one machine-checkable part.

- **One act.** Arrange, act, assert. Two acts on the subject before the
  assertions means two tests wearing one name.
- **One subject.** Asserting fourteen fields of one snapshot is one
  thing. Asserting a snapshot *and* a log *and* a file on disk is three,
  and the test will fail for reasons its name does not cover.
- **The name matches the body.** Judge this by reading. A detector for it
  was built and thrown away: on this tree it fired on 22% of all tests,
  because a name like `Describe_...` reaches its subject through a scene
  object rather than a call spelled `describe`. A detector that noisy
  teaches people to ignore it, which is the failure this skill exists to
  prevent.

When a test is about three things, split it. Do not delete two of the
assertions — they were presumably there for a reason.

## Question 4: does anything else already cover it?

Redundancy is a different question from failure, and answering the first
tells you nothing about the second. Three layers, cheapest first, each
narrowing the candidate set for the next.

**A1 — assertion subsumption (static, no build).** Test A's assertions
are a subset of B's over the same subject. Run `find_redundant_tests.py`.
Candidates only.

**A2 — coverage subsumption.** `cov(A) ⊆ cov(B)`. A *necessary*
condition: if A covers a line nothing else covers, A is not redundant,
full stop. Run `find_coverage_redundancy.py --module libs/wfc`. It also
answers a question you need anyway — which tests are load-bearing for
the 100% gate — so a deletion cannot silently drop a branch.

Measured on `libs/wfc`, 70 tests: 11 load-bearing, 44 subsumed, 1
covering no production line, 14 neither. So the suite could be
substantially smaller *by coverage* — and the same run shows why that
number must not be acted on directly:

```cpp
// Identical coverage. Mutually subsumed. Both must stay.
TEST(SolverConstructionTest, Ctor_ThrowsOnAZeroValueWeight)      // 0.0
TEST(SolverConstructionTest, Ctor_ThrowsOnANegativeValueWeight)  // -1.0
```

Two rejections of the same guard by different inputs run the same lines
and document different contracts. Coverage cannot tell them apart, and
neither can a line count. That is the whole reason A3 exists.

**A3 — mutation kill-set subsumption.** `kill(A) ⊆ kill(B)`. The only
criterion that means *deleting A loses no fault detection*. Run it
against the A2 survivors so the expensive layer sees a small set.

**The tie-break decides more than the score.** When two tests are
mutually redundant, keep the one that is more focused and better named,
not the one that incidentally kills more mutants. Without that rule a
minimisation pass will delete a clean unit test and keep a sprawling
integration test, and the suite gets worse while the numbers improve.

**A candidate is not a verdict.** Two tests that die to the same mutants
today can diverge tomorrow; a name documents an intent; and on a tree
gated at 100% branches, some tests exist mainly to hold a branch up.
Deleting a *worthless* test dropped a real branch twice on this tree.

## Isolation

Two tests that share state are one test with a confusing name. This tree
is clean and should stay that way:

- `antwika_bundle_test` registers every test as its own `ctest` entry, so
  CI already runs each in a separate process.
- `check_isolation.sh` runs every binary with all its tests in *one*
  process, shuffled, repeated. That is the run that exposes order
  dependence, and all 48 binaries pass it.

Do not introduce file-scope mutable state in a test file, and take
temporary paths from `antwika::testing::scratchPath` so two tests running
in parallel cannot collide.

## The method: mutation is the arbiter

Reading produces an argument. Mutation produces an answer, and when they
disagreed on this tree, mutation was right every time.

Break the production code on purpose and see which tests notice:

```sh
# Does anything actually guard adjacency pruning?
#   -> make prune() a no-op, rebuild, run.
#   Result on this tree, before the sweep: 70 of 70 tests passed.
```

Measured here, one mutation at a time:

| Mutation | Tests that noticed |
|---|---|
| Deleted the weighted-entropy branch | 1 of 70 |
| Made adjacency pruning a no-op | **0 of 70** |
| `Viewport::operator==` → address comparison | 0 of the 16 `ViewportTest`s that should have |
| Perturbed the world-generator seed | 1 of 16 |

Every mutation written during that review found a hole. That is the
argument for doing this rather than trusting a careful read.

`mutate.py` automates it: it samples mutation sites in one module,
rebuilds that module's test target per mutant, runs it, and restores the
file. On `libs/wfc` after two review passes it scores 85%, with two
survivors — and reading them is the point, because **a survivor means one
of three things and only one of them is a missing test**.

**Equivalent — the mutant changes nothing observable.** Not a gap.

```cpp
// Solver.cpp: a dedup flag for the propagation worklist.
// Flipping it to false lets a constraint be enqueued twice: more work,
// same answer. No test should be written for this.
queued[index] = true;
```

**Undefined behaviour a unit test cannot see.** Not a gap in the tests —
a gap in the build.

```cpp
// Domain.cpp: `<` to `<=` writes one past the end of a vector<bool>.
// No assertion catches that; a sanitizer build would.
for (std::size_t i = 0; i < bits.size(); ++i)
```

This repository has no sanitizer preset, so mutants of this shape will
keep surviving. That is worth fixing once at the build level rather than
chasing test by test.

**A genuine gap — the behaviour changed and nothing noticed.** Write the
test. This is the category the sweep found repeatedly before it was
fixed: making `AdjacencyConstraint::prune` a no-op left 70 of 70 tests
green.

Triage every survivor into one of those three before touching a test.

## Tools

Run from `tools/`. Each prints `path:line | test | category | detail` and
exits non-zero when it finds anything.

| Tool | Question | Precision |
|---|---|---|
| `find_unfailable_tests.py` | 1 | high — every hit needs a human, but few hits |
| `find_unestablished_premises.py` | 2 | medium — triage list, highest yield before a semantic pass |
| `find_unfocused_tests.py` | 3 | medium — `--subjects N` sets the threshold |
| `find_redundant_tests.py` | 4 (layer A1) | candidates only, never a verdict |
| `check_isolation.sh` | isolation | exact — a failure is a real order dependence |
| `mutate.py` | 1 and 4, the arbiter | exact, modulo the three survivor kinds above |
| `find_coverage_redundancy.py` | 4 (layer A2) | exact on coverage; coverage is only necessary, never sufficient |

`mutate.py --module libs/wfc --limit 25` takes minutes on a small
library and is the only tool here that answers question 1 outright
rather than nominating a candidate. Start a review with the cheap
detectors, finish an argument with mutation.

**A mutation run is always bounded, and does not rely on you to say so.**
It rebuilds once per mutant, which is fine for one run and fatal for
several: eight concurrent unbounded runs on a 24-core box ask for 192
compilers, and the box stops responding long before it finishes. So
`mutate.py` pins itself before anything forks, for every module, library
and app alike. Three ways the CPU set is settled, first one wins:

```sh
python3 mutate.py --module libs/ui --cores 0-1   # this run gets two CPUs
MUTATE_CORES=0-1 python3 mutate.py --module apps/game   # every run does
python3 mutate.py --module libs/ui               # all but two, by default
```

Pinning is what makes the limit hold: make, every compiler and the test
binary inherit the affinity mask, so the kernel enforces it rather than
the build tool. `--jobs` sets `cmake --build -j` alone, which bounds the
build if make cooperates and bounds nothing else; it defaults to the
pinned core count, so the two cannot drift apart. The header line prints
the set and what chose it before the run spends any of it:

```
cores       2 of 24 (0-1) [MUTATE_CORES]
jobs        2
```

For a fan-out, give each worker a disjoint slice — `MUTATE_CORES=0-1`,
`2-3`, `4-5` — and the total is bounded by construction.

**It mutates the tree you are standing in.** A worktree checks out its
own copy of these tools, but an agent in a worktree can still reach for
the project root's copy by absolute path, and that must not mutate the
project root. The tree comes from `git rev-parse --show-toplevel` on the
working directory, never from where the script lives; `--repo` overrides,
and the header names the tree and says so when the two differ:

```
repo        /path/to/worktree
            (script lives in /workspaces/antwika; that tree is not touched)
```

**A killed run restores.** SIGINT, SIGTERM and SIGHUP all put the source
back, and so does an exception nothing caught — only SIGKILL can leave a
mutant on disk. If a run ever is `kill -9`ed, check `git status` before
trusting anything the tree builds.

Both build-driven tools work one module at a time on purpose: a whole
tree pass is hours, a module is minutes, and findings arrive while they
are still actionable. `find_coverage_redundancy.py` collects profiles
with `gcov --json-format --stdout` rather than a `gcovr` pass, which is
the difference between 0.03 seconds and 10 per test — a whole-module
sweep is not practical any other way.

The detectors cover the syntactic half. Questions 2 and 3 and layer A2
need reading or running; no grep substitutes for opening the production
code and asking what edit would survive.

## The checklist

Before accepting any test:

1. Name the edit that turns it red. If none, delete it or pin a real
   expectation.
2. Does it compare a computation to itself? Pin the value instead.
3. Could a payload that ignores the named property still pass?
4. Does the body contain an assertion, and does it reference something
   the code under test produced?
5. Would the assertion hold if the production code returned its input?
6. Is a value compared to itself where two equal values would be
   stronger?
7. Could every assertion be skipped — inside a loop or a branch that
   never runs?
8. Would it pass if the production code produced nothing at all?
9. Does it fail for exactly one reason, and is that the reason its name
   gives?
10. Does another test in the file already pin this behaviour?

Items 1 to 7 are about the test being real. Items 8 to 10 are about it
being worth keeping.

## Where this came from

A tree-wide sweep of 578 files and 5,521 tests. The mechanical pass
fixed 62 defects; a second, semantic pass over the same files by fourteen
independent readers found roughly 60 more, in files the first pass had
already declared clean — including one in a library reviewed end to end
and reported as swept. A careful reader has a hit rate below one, which
is why the checklist is written down and why the arbiter is mutation
rather than argument.
