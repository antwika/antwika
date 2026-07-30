# Confirming a branch is unreachable before excluding it

`// GCOVR_EXCL_LINE` removes a line from every coverage denominator: lines, functions, and branches.
That makes it easy to misuse as a shortcut past an inconvenient test.
This guide is the procedure for using it only when a branch is *actually* unreachable, not merely annoying to reach.

The bar: before adding the marker, you must be able to answer concretely — "what would have to happen at runtime for this branch to be taken, and can this codebase ever cause that without deliberately breaking something unrelated?" If you can't answer that with a provable no, write a test instead.
Most misses turn out to be real gaps, not exclusion candidates — see [Where this guide came from](#where-this-guide-came-from) below.

## Procedure

### 1. Get exact missing lines/branches, not just the summary percentage

`gcovr`'s `--print-summary` only gives per-file percentages.
Export JSON instead and read it programmatically:

```sh
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' \
    --exclude-throw-branches --exclude-unreachable-branches \
    --gcov-executable gcov --json --json-pretty \
    -o coverage.json build-coverage
```

Then scan `coverage.json` for lines/branches with `count == 0` — **but filter out anything already carrying `"gcovr/excluded": true"` first**.
An already-excluded line still reports `count: 0` in the JSON; treating every zero-count entry as a fresh miss produces false positives for lines someone already handled correctly.

### 2. Do a clean rebuild before trusting any number

Editing an instrumented source file and rebuilding incrementally leaves a stale `.gcda` behind.
The tell is `gcov`'s own warning:

```
libgcov profiling error: .../Foo.cpp.gcda:overwriting an existing
profile data with a different checksum
```

It's not fatal, but don't trust numbers from that run.
Wipe and reconfigure first:

```sh
rm -rf build-coverage
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
```

### 3. Read the annotated source, not just the summary

Find the `.gcno`/`.gcda` pair for the translation unit (for a header-only template, any TU that instantiates it will do), then:

```sh
gcov -b path/to/Foo.cpp.gcda
```

This writes `Foo.cpp.gcov` next to it: a per-line annotated copy of the source showing execution counts, and — critically — per-branch tags like `(throw)` / `(fallthrough)`, and calls marked `never executed`.
The tags are what actually justify an exclusion; a bare `count == 0` in the JSON never does on its own.

### 4. Recognize the concrete signatures of genuine unreachability

**(a) A branch tagged `(throw)`.** This is the exception-unwind edge generated for a call that could throw — allocator failure inside `std::vector::push_back`, `std::string` construction, and similar.
It's taken only if that specific call actually throws.

`gcovr`'s `--exclude-throw-branches` is supposed to strip these automatically, but don't assume it always does — it depends on `gcovr`'s own classifier agreeing with `gcov`'s tag, and the two can disagree.
Trust the raw `gcov -b` tag over `gcovr`'s JSON when they conflict.

**(b) A whole line reads `=====` with a call marked `never executed`, usually a bare closing `}`.** This is the exception-cleanup landing pad that destructs a local object — typically one holding a `std::vector` — if the function unwinds.
It never shows as covered no matter how many normal, non-throwing calls exercise the function, because that destructor call only exists on the unwind path.

**(c) A branch direction that's mathematically impossible given the surrounding code**, independent of exceptions entirely.
This one needs the most care, because it's a genuine logic argument, not a tag you can just read off — write out why the "other" direction can never be taken, and be suspicious of your own reasoning if the argument takes more than a sentence or two.

### 5. Exclude the exact line the count is attributed to

Add `// GCOVR_EXCL_LINE` on the line `gcov` actually attributes the zero count to — not necessarily the line you'd guess.
A statement split across several lines gets its count attributed to a specific sub-expression's line, and a marker on the wrong line (e.g. a call's closing `);` instead of where the call itself starts) silently does nothing.
Confirm placement by rebuilding (step 2) and re-checking the JSON (step 1).

### 6. If none of (a)–(c) apply, it's a real gap — write a test

## Worked examples

### (a) `(throw)`-tagged allocation branch

`src/libs/ecs/src/SystemScheduler.cpp`:

```cpp
PhaseId SystemScheduler::createPhase(std::string_view name)
{
    phases.push_back(Phase{std::string(name), {}}); // GCOVR_EXCL_LINE
    return static_cast<PhaseId>(phases.size() - 1);
}
```

`gcov -b`'s annotation on that line showed two branches tagged `(throw)` at 0%, covering `std::string`'s construction and the `vector::push_back` reallocation — both only reachable if an allocation actually fails.

### (b) Exception-cleanup landing pad at a closing brace

`src/apps/life/src/Board.cpp`:

```cpp
Board readBoard(const World &world, const Grid &grid)
{
    Board board{ /* ... */ };
    for (/* ... */) { /* ... */ }
    return board;
} // GCOVR_EXCL_LINE
```

`readBoard` is called dozens of times across the test suite, yet the closing `}` showed `=====` with `call 0 never executed`.
That call is the destructor for the local `Board` (via its `std::vector<bool> alive` member), reachable only if something between construction and `return` throws — nothing here ever does.

The identical pattern showed up in `src/libs/ecs/include/antwika/ecs/View.hpp`, on the closing braces of `View`'s constructor and of `smallestEntitiesOf` — both locally hold a `std::vector<Entity>`.

### (c) Mathematically impossible branch direction

`src/libs/ecs/include/antwika/ecs/View.hpp`:

```cpp
std::size_t smallestSize = std::numeric_limits<std::size_t>::max();

auto consider = [&](auto *storage)
{
    const auto entities = storage->entities();
    if (entities.size() < smallestSize) // GCOVR_EXCL_LINE
    {
        smallestSize = entities.size();
        smallest.assign(entities.begin(), entities.end());
    }
};
(consider(storages), ...);
```

`smallestSize` starts at `max()`.
The *first* `consider()` call in the fold — for any instantiation, with any storage sizes — can never take the "not smaller" branch, because nothing is ever bigger than `max()`.
No test can make that comparison false without changing the algorithm itself.

The same file's `erase_if` predicate had a related but distinct case: for a *single*-parameter `View<T>` (e.g. `View<Cell>`), `matching` is drawn directly from that one storage, so the predicate's `!storage->contains(entity)` can never be true — the entity being checked always came from that exact storage.
That's specific to the single-parameter case; the same line is fully exercisable (and tested) for two-or-more-parameter views, where an entity from the smallest storage can genuinely be absent from another one.

## Anti-patterns

- Don't exclude a line because the test is inconvenient to write, or the scenario is rare.
  Rare is not the same as impossible.
- Don't exclude more than the specific line at fault — a whole function's declaration, or an unrelated caller, should never carry the marker for one internal branch's sake.
- Don't trust `--exclude-throw-branches` / `--exclude-unreachable-branches` alone.
  They help, but they classify *branches*; the closing-brace landing pad in (b) above is a *line* miss with a `never executed` call, and no branch-level flag touches it.
- Don't skip the clean-rebuild step.
  A stale `.gcda` can make an already-fixed gap look unresolved, or an unresolved one look fixed.

## Where this guide came from

Written after a pass that took GNU coverage from 93.0% lines / 96.6% functions / 83.2% branches to 100% / 100% / 100%.
Of roughly 20 initially missing lines and branches, most turned out to be real, previously untested behavior — a dead-entity throw path, a "component the entity never had" no-op path, a fold-expression short-circuit direction nothing had exercised — and got closed with new test cases, not exclusions.
Only the handful matching (a)–(c) above were excluded.
