# Contributing

[`docs/STYLE_GUIDE.md`](../docs/STYLE_GUIDE.md) is the authoritative coding style — naming, formatting, includes, error handling, testing and CMake conventions.
This page covers the working practices around it rather than restating any of it.

## Work in a git worktree, never in the primary checkout

Before making any change, create and enter a dedicated worktree:

```sh
git worktree add .worktrees/<task> -b <task>
```

Do all editing, building and testing there, and only merge back when the work is done.
This keeps `main` clean and lets several tasks build in parallel without clobbering each other's `build/` directory.
`.worktrees/` is the agreed home for them and `.gitignore` covers it, so a worktree and its build output never appear as untracked state in the primary checkout.

## Read the blog post before changing an abstraction

[`blog/`](../blog/) holds after-the-fact write-ups of *why* a piece was built the way it was.
They usually explain a constraint that is not obvious from the code, so read the relevant one before changing a library's core abstraction.
[`REQUIREMENTS.md`](../REQUIREMENTS.md) is the other half of that: every design constraint stated as a MoSCoW requirement, including the explicit "won't have" list that records what was considered and deliberately left out.

## Checker scripts

All three are enforced in CI and all three run locally the same way:

```sh
python3 scripts/check_unused_test_doubles.py     # every mock/fake is used
python3 scripts/check_one_sentence_per_line.py   # comments and markdown prose
python3 scripts/check_line_length.py             # 80-char limit, src/ + scripts/
```

Run them before committing; they are fast and catch most CI failures.
Each has its own test under `scripts/tests/`, run with e.g. `python3 scripts/tests/test_check_line_length.py`.

The one-sentence-per-line rule applies to this wiki too.

## Tests

Tests are written with GoogleTest, registered with CTest, and live in the module's own `tests/` directory.
Write them alongside the behaviour rather than bolting them on afterwards.

While iterating, run a single binary or a filter rather than the whole suite:

```sh
ctest --test-dir build -R antwika_replay_tests --output-on-failure
build/bin/antwika_replay_tests --gtest_filter='ReplayReaderTest.*'
```

Run the full `ctest --test-dir build --output-on-failure` before considering a change done.

Every mock and fake header under a `tests/{mocks,fakes}/include` directory must be included by at least one `.cpp` file, which is what `check_unused_test_doubles.py` enforces.

## Coverage

CI requires **100% line, function and branch coverage** on the GNU leg, checked by `scripts/check_full_coverage.py`.

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
gcovr --root . --filter 'src/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

The LLVM branch percentage is informational only, because LLVM's `gcov` emulation cannot tag compiler-generated exception-unwind branches the way GCC's can.
MinGW carries no coverage instrumentation at all.

Excluding a line with `GCOVR_EXCL_LINE` is a last resort: it needs a comment saying why, and it may only be used after real, testable gaps have been covered by actual tests.
Follow [`docs/confirming-unreachable-branches.md`](../docs/confirming-unreachable-branches.md) first.
Each app's `main.cpp` is the one file left out of the report.

## Commits and releases

Commits follow [Conventional Commits](https://www.conventionalcommits.org/) — `feat(scope): ...`, `fix(scope): ...`, `docs(scope): ...`, `test(scope): ...`, `perf(scope): ...`, `style(scope): ...` — scoped to the module or modules touched, with a lowercase subject and a body saying *why*.
Releases are cut by `semantic-release` from that history, so `CHANGELOG.md` is never hand-edited and a version is never bumped manually.

## Design expectations

A few rules come up in review often enough to be worth naming here.

- One exception type per failure category, each catchable on its own.
- No global state; interfaces are `I`-prefixed and injected.
- Adding a concern to an existing, tested class prefers composition — a decorator — over modifying that class.
- An interface with a single implementation is kept when it lets a class be unit-tested against a mock in isolation.
- Comments default to absent, added only when the *why* is non-obvious; Doxygen blocks on public API are the exception and are kept regardless.
- A feature's diff goes through independent review passes (reuse, simplification, efficiency, altitude), with a recorded rationale for anything flagged and deliberately left alone.
