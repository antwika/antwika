---
name: finish-remaining-tests
description: Drive the tree from partial coverage back to the 100% lines/functions/branches the gate demands — find the gaps, then classify each one as untested behaviour, dead code, or a compiler artefact that no test can reach, because the three want opposite fixes. Use when coverage is short, when scripts/check_full_coverage.py fails, or when a new uncovered branch appears after adding library code.
---

# Finishing the remaining coverage

`scripts/check_full_coverage.py` demands 100% of lines, functions **and**
branches. When it is short, the instinct is to write a test for every
gap. That instinct is wrong about half the time, and acting on it costs
whole cycles.

**A gap is one of three things, and only one of them wants a test.**

| Kind | What it is | Fix |
|---|---|---|
| Untested behaviour | an input exists that reaches the arm; nobody wrote it | write the test |
| Dead code | no input reaches it, because every caller settled the condition first | delete the guard |
| Compiler artefact | no input reaches it, because the arm is not in your source at all | restructure, else `GCOVR_EXCL_LINE` |

Classify before you write anything. The last two are unreachable by
construction, so a test aimed at either burns a cycle and leaves the
branch red.

## The cycle is minutes, so measure rather than reason

Everything below assumes `-j24` on every step. Measured on this machine,
6289 tests, 745 measured files:

```sh
rm -rf build-coverage && cmake --preset conan-coverage   # seconds
cmake --build build-coverage -j24                        # 2m40s clean
find build-coverage -name '*.gcda' -delete
ctest --test-dir build-coverage -j24                     # 15 SECONDS
python3 .claude/skills/finish-remaining-tests/tools/covgaps.py   # ~1m
```

A per-file check is under a second. There is no reason to batch a dozen
guesses into one run — fix one file, verify it, move on. Cheap
measurement is the whole method: **prefer one experiment to ten minutes
of reasoning about what the compiler probably did.**

If a note anywhere claims this cycle takes hours, it was measured
without `-j24` on `ctest` and is wrong by two orders of magnitude.

## Step 1: a baseline you can trust

Two hygiene rules, both learned by getting them wrong.

**Recompiling one object invalidates its coverage data for every binary
that links it.** A static library object accumulates counts from every
test executable that links it. Rebuild one target and the others still
hold the pre-edit object, so the report is built on a mixture and the
gcov tool complains about a stamp mismatch. The fix is always the same
three commands, in order:

```sh
cmake --build build-coverage -j24          # no --target, so all dependents relink
find build-coverage -name '*.gcda' -delete
ctest --test-dir build-coverage -j24       # no -R, so every contributor runs
```

A per-target build plus a filtered `ctest -R` measures stale objects and
tells you a comfortable lie.

**`gcovr` scatters `*.gcov` intermediates into its working directory.**
Run it from the checkout root and you get about a hundred
`name##hash.gcov` files in the tree, and eventually a hard failure to
resolve a working directory. `covgaps.py` runs `gcovr` from a scratch
directory. If you invoke `gcovr` by hand, do the same, and check
afterwards:

```sh
find . -name '*.gcov' -not -path './build*' | wc -l    # must be 0
```

## Step 2: list the gaps

```sh
python3 .claude/skills/finish-remaining-tests/tools/covgaps.py
```

No arguments sweeps the tree and prints one block per file with a gap —
uncovered lines, and the lines carrying untaken branches with how many.
Exits 0 when clean, 2 when it finds gaps. It honours `GCOVR_EXCL_LINE`,
the same filters as `scripts/coverage.sh`, and the `GCOV_EXECUTABLE`
environment variable, so it agrees with the gate on whichever toolchain
you configured.

Pass paths for the fast loop:

```sh
python3 .../covgaps.py src/libs/ui/src/Resolve.cpp    # ~0.5s
```

It scopes `gcovr` to just the directories holding that file's coverage
data, which is why it is instant. A **header** has none of its own, so it
falls back to scanning every object directory and costs about as much as
the whole-tree sweep — for headers, just read the sweep.

**Silence is never success.** The sweep prints `No gaps: N measured files
are fully covered.` and refuses to exit 0 if it matched no files at all.
An earlier version of this tool passed a filter that resolved against
the wrong directory, matched nothing, and reported a clean tree while a
real gap sat in `Sudoku.cpp`. If a coverage tool tells you the tree is
clean, check that it also tells you how much it looked at.

## Step 3: classify each gap

Work in this order. Each step is cheaper than the one after it, and most
gaps are settled by the first two.

### 3a. Read the condition and name the input

Ask one question: **what input makes this arm run?** Name it concretely
— a value, a state, a sequence of events.

If you can name it, it is kind 1. Write that test. Most real gaps are
found and fixed here, and they are usually specific and unglamorous: a
list short enough to need no scrolling, a press landing under an open
menu, a config member nobody compared.

If you cannot name any input, do not conclude the branch is unreachable
yet — you may simply not understand the code. Go to 3b.

### 3b. Check every caller

For a guard at the top of a function, read every call site. If all of
them already establish the condition, no input can reach the guard, and
it is kind 2: **delete it.**

`GridSink::endRoadDrag` opened with `if (!drag.active()) return;` and its
one caller had just tested `drag.active()` before calling. There is no
test that reaches that line, and there should be no line.

Prefer deleting the guard to adding a public entry point that exists
only so a test can reach it. If the function is genuinely public API and
the guard is a real contract, that is kind 1 and 3a should have found
the input.

### 3c. Suspect the expression, not the logic

If the branch has no counterpart in your source at all — you cannot
point at the `if` it belongs to — it is probably kind 3.

Two tells, both source-level:

- **The count is suspiciously uniform.** The same construct in nine
  different files reporting the same number of untaken branches is a
  property of the construct, not of nine independent testing oversights.
- **The line is not a condition.** A branch reported on `};`, on a member
  initialiser, or on a `}` is not a branch you wrote. Compilers attach
  cleanup and unwind paths to whichever line closed the expression.

These arise from constructs that build owning members inside one
expression, where the compiler emits a cleanup path for the
already-constructed members in case a later one throws. Known
signatures:

| Signature | Restructure to |
|---|---|
| brace-init returning an aggregate with owning members | build it member-by-member into a named local, then return it |
| `std::vector<std::string> v{ "a" + b };` | `std::vector<std::string> v;` then `v.emplace_back(...)` |
| brace-init nlohmann json (`x = {{"k", 1}}`) | assignment-style, array entries as named locals |
| a reference-only aggregate temporary's head line | hoist the spec to a named local |
| designated-init aggregate holding a container member | pass owning members as separate by-value parameters |

### 3d. The decisive experiment: restructure and re-measure

This is what separates kind 1 from kind 3, and it needs no knowledge of
your compiler or target.

Rewrite the expression per the table — same behaviour, different shape —
then rebuild and re-measure that one file. Watch the **branch total**,
not the percentage:

- **The total drops and the gap is gone** → the branches were an artefact
  of the expression. Keep the restructure. Done.
- **The total is unchanged and the gap remains** → the branch is real.
  Go back to 3a; you have not found the input yet.
- **The total drops but a gap remains** → partly both. Repeat.

A real branch cannot be removed by rewriting an expression into an
equivalent one. A generated cleanup path can. That asymmetry is the
whole test, and `covgaps.py` prints the total on every run so you can
see it move.

Reordering an owning member last in an aggregate halves these branches,
because only the final member needs no cleanup — but a struct with two
owning members never reaches zero that way, so it churns a public header
for nothing. Restructure the expression, or exclude.

### 3e. Exclude, last

When no restructure reaches it, `GCOVR_EXCL_LINE` is sanctioned —
`docs/STYLE_GUIDE.md` lists it as one of three permitted tool markers.
Two conditions before you write one:

1. You have done 3d and the branches survived every equivalent shape.
2. **No real branch shares the line.** Exclusion is per line, so check
   what else sits there. If the line holds a condition you wrote,
   excluding it silently drops that branch too.

An exclusion is a claim that no input reaches the line. Be able to say
why in one sentence, in the commit message.

## Step 4: verify that file, then move on

```sh
cmake --build build-coverage -j24
find build-coverage -name '*.gcda' -delete
ctest --test-dir build-coverage -j24
python3 .../covgaps.py <the file you just fixed>
```

Confirm the file is clean before starting the next. Verifying one at a
time is what catches a wrong hypothesis after one cycle instead of after
twelve.

Finish with the real gate, not the tool:

```sh
./scripts/coverage.sh --summary coverage-summary.json
python3 scripts/check_full_coverage.py --summary coverage-summary.json
rm -f coverage-summary.json
```

`check_full_coverage.py` compares exact covered/total counts, so its OK
is trustworthy.

## Tests written to close a branch need extra scrutiny

Hold every one of them to `quality-check-tests`. A test written for a
branch is the test most likely to assert nothing, because the branch —
not the behaviour — was the goal. Name the behaviour in the test name
and assert on it. If you cannot say what the branch *means*, you are
probably in kind 2 or 3 and should not be writing a test at all.

Two recurring shapes:

- **A defaulted `operator==` needs one differing value per member.**
  Comparing equal values plus one differing member leaves an untaken arm
  for every other member. `expectMemberCompared` in `ValueEqualityTest`
  is the idiom. A gap here usually means a member was added to the struct
  and never added to the test — `GameConfig::spreadDelayTicks` sat
  missing among eighteen listed members.
- **A `std::string` member also needs a value too long for the inline
  buffer.** Short names leave the heap-allocating arm of every copy
  untaken; one string over about fifteen characters covers it.

## A trap when reading branch reports

A multi-line `&&` chain reports all of its branches on **one** line, and
not necessarily the line the sub-expression is written on. A gap
reported on the line holding a function call may belong to a condition
three lines above it.

So before writing a test for "the call that never returned false",
account for every condition in the chain and work out which one has an
arm nobody took. In `Resolve.cpp` the missing arm looked like an
untested `contains()` call and was really the negated flag at the top of
the chain — which pointed at a quite different test.

## The checklist

Before writing a single test for a coverage gap:

1. Is the baseline trustworthy — full rebuild, wiped coverage data, full
   `ctest`? A stamp mismatch means no.
2. Does the tool report how much it measured, or only silence?
3. Can you name the input that reaches the arm? If yes, write it.
4. If no: does every caller already settle the condition? Then delete
   the guard.
5. If neither: does the branch have a counterpart in your source at all?
   Restructure the expression and watch whether the branch total drops.
6. If you are about to exclude a line, did an equivalent shape fail to
   remove it, and does a real branch share the line?
7. If you are about to write a test, name the edit that turns it red.

## Where this came from

A sweep on 2026-08-07 that found 18 files short: 3 lines and 25
branches. Nineteen of those branches were compiler artefacts, three were
dead code, and only four were behaviour anyone could test — so the
default instinct would have been wrong for four fifths of the work.

The session lost a cycle assuming the nine app gaps were untested string
handling, built a test across nine apps to force the arm, and watched
the branches stay red. Restructuring the expression made the branch
total fall instead — the arms were never in the source. Step 3d is that
experiment, generalised, and it is placed before any test-writing so the
cycle is not lost twice.
