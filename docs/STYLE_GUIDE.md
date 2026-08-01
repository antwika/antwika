# Coding style guide

This collects the conventions already visible across the codebase into one place, so they're easy to follow instead of only being learned from review comments.
It doesn't invent new policy — see [`REQUIREMENTS.md`](../REQUIREMENTS.md) for the must/should/could/won't statements this style serves, and
[`blog/002-writing-the-rules-down.md`](../blog/002-writing-the-rules-down.md)
for the story behind the two rules that are enforced by scripts rather than just written down.

Where a rule is enforced by a CI-run script or compiler flag, that's called out explicitly.
Everything else is convention to follow by hand.

## Language and toolchain

- C++23, built with CMake (>= 3.28) and Conan, tested with GoogleTest registered through CTest.
- Compiler warnings (`-Wall -Wextra -Wpedantic -Wsuggest-override`) are treated as errors by default (`ANTWIKA_WARNINGS_AS_ERRORS`, on by default).
- Checker scripts under `scripts/` are plain, type-hinted Python 3.

## Project layout

Each app under `src/apps/<name>` and each library under `src/libs/<name>` owns its own `CMakeLists.txt`, `include/`, `src/`, and `tests/` directory.

- Public headers live under `include/antwika/<module>/` and get installed; code outside the module includes them as `<antwika/module/Header.hpp>`.
- Private headers live directly under `src/`, next to the `.cpp` file that needs them, and are never installed.
- Test doubles live under `tests/mocks/include/antwika/<module>/mocks/` or `tests/fakes/include/antwika/<module>/fakes/`, mirroring the public include layout one level deeper.

## Formatting

- 4-space indentation, no tabs.
- Allman braces everywhere — the opening brace goes on its own line, for namespaces, classes, functions, and control flow alike.
- A namespace's body is indented one level relative to the `namespace { ... }` braces; the closing brace carries a `// namespace antwika::module` comment.
- `#pragma once` at the top of every header, not include guards.
- **Hard 80-character line limit** on every line of `src/**/*.cpp`, `src/**/*.hpp`, `scripts/*.py`, and `scripts/tests/*.py`.
  Wrap long designated-initializer structs, function calls, and expressions across multiple lines (one argument/field per line reads best) rather than exceeding it.
  Enforced by `scripts/check_line_length.py` in CI.

```cpp
namespace antwika::module
{

    class Example final
    {
    public:
        explicit Example(int value);

    private:
        int value;
    };

} // namespace antwika::module
```

## Includes

Group includes with a blank line between each group, alphabetized within a group:

1. This file's own matching header, quoted (`.cpp` files only).
2. Third-party library headers (`<gtest/gtest.h>`, `<gmock/gmock.h>`).
3. C++ standard library headers, angle-bracketed.
4. Other modules' public headers, angle-bracketed
   (`<antwika/other-module/Thing.hpp>`).
5. This module's own headers — public ones by their full installed path, private ones by bare filename — quoted, last.

```cpp
#include "antwika/game/GameStateReducer.hpp"

#include <string>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
```

## Naming

- **Namespaces**: lower-case, matching the module (`antwika::game`, `antwika::replay`).
  A group of related constants gets its own nested namespace, e.g. `antwika::game::events`.
- **Types** (classes, structs, enums): `PascalCase`, and the file name matches the type it declares exactly (`GameStateReducer.hpp` declares `class GameStateReducer`).
- **Interfaces**: prefixed with `I` (`IEventSink`, `ILogger`, `ISystem`), pure abstract, with `virtual ~IThing() = default;` as the first member.
- **Methods and functions**: `camelCase`.
- **Member variables**: `camelCase`, no `m_`/trailing-underscore decoration.
  Private members are declared after the public interface, under their own `private:` section.
- **Constants**: `k`-prefixed `camelCase`, declared `inline constexpr`
  (`kTick`, `kScoreIncrement`, `kNormalPriority`).
- **Event name strings**: lower-case, dot-namespaced
  (`"engine.tick"`, `"game.score_increment"`).
- **Enum class enumerators**: `PascalCase` (`Level::Warning`).
- **Error types**: `<Something>Error`, deriving from `std::runtime_error`
  (see [Error handling](#error-handling)).
- **Test doubles**: `Mock<Type>` / `Fake<Type>`, in a `::mocks` / `::fakes` sub-namespace of their module.

For a strongly-typed integer id or handle, prefer an empty scoped enum over its backing integer plus a `rawValue()` accessor, rather than a plain integer typedef — see `antwika::scheduler::Priority` and `antwika::scheduler::JobId` for the idiom:

```cpp
enum class Priority : std::uint8_t
{
};

inline constexpr Priority kNormalPriority{1};

[[nodiscard]] constexpr std::uint8_t rawValue(Priority priority) noexcept
{
    return static_cast<std::uint8_t>(priority);
}
```

## Comments and documentation

- Comments default to **absent**.
  Add one only when the *why* isn't obvious from the code itself: a hidden constraint, a subtle invariant, a workaround for a specific issue, or behavior that would otherwise surprise a reader.
  Never explain *what* the code does — a well-named identifier already does that.
- **Exception**: every public API surface (interfaces, classes, public methods) carries a Doxygen `@brief`/`@param`/`@return`/`@throws` block, kept regardless of whether the *why*-only rule above would otherwise justify one.
  These document the *what*, for generated reference docs.
- **One sentence per line**, however long that line gets, in every `//` and `#` comment and in every markdown document in the repository: `README.md`, `CLAUDE.md`, `REQUIREMENTS.md`, `blog/*.md` and `docs/**/*.md`.
  Never let two sentences share a line, and never wrap one sentence across two lines.
  Enforced by `scripts/check_one_sentence_per_line.py` in CI.

```cpp
/**
 * @brief Sends events to whatever consumes them.
 */
class IEventDispatcher
{
public:
    virtual ~IEventDispatcher() = default;

    /**
     * @brief Dispatch an event to its consumers.
     * @param event The event to dispatch.
     */
    virtual void dispatch(Event event) = 0;
};
```

## Class and API design

- Follow SOLID, with small, single-purpose interfaces.
- Classes are `final` by default.
  Only make a class a base when it's a genuine interface — pure virtual — meant to be implemented by other types.
- Constructors that take a single argument (and multi-argument ones where an implicit conversion would be unwanted) are marked `explicit`.
- Non-mutating query methods are marked `[[nodiscard]]`.
- To add a new concern to an already-tested class, prefer composition (e.g. a decorator) over modifying that class directly.
- An interface with only one implementation is still worth keeping when it lets the class under test be exercised against a mock/fake in isolation
  (`IEventCodec`, `IFormatter`, and similar).

## Resource management and ownership

Every resource is released by the destructor of the object that owns it.
There is no `new`, `delete`, `malloc`, or `free` anywhere under `src/`, and no cleanup step a caller has to remember.

- **Borrowed collaborators are reference members.** A class that *uses* a collaborator without owning it stores it as `Thing &thing`, and its `@param` says "Must outlive this object" — see `Logger`, `Engine`, `EngineLoop`.
  This is the default; it's what the constructor-injection style throughout the codebase is built on.
- **Owned resources are `std::unique_ptr` members.** Reach for `std::shared_ptr` only where type erasure genuinely needs it — the one instance is `World`'s `shared_ptr<void>` component pools, where the control block is what remembers the concrete `ComponentStorage<T>` destructor.
  A `unique_ptr<void>` there would leak.
- **Prefer transferring a `std::unique_ptr` over documenting a lifetime rule.** When a class must hold on to something a caller heap-allocated, take it by `std::unique_ptr` value rather than storing a raw pointer and writing the rule in a comment — a doc comment can't be violated at compile time, and a `unique_ptr` parameter can't be got wrong.
  See `Scheduler::schedule`, which offers both and says which to reach for.
- **Rule of Five, explicitly.** Any class holding a reference member or owning a resource declares all four copy/move operations — in this codebase always `= delete`, since aliasing borrowed collaborators or relocating what a live borrower points at is never what's wanted.
  A reference member already deletes both assignment operators implicitly, but not the copy and move constructors — declaring all four is what makes the intent checkable rather than incidental.
  Plain value types (`GameState`, `Worker`, `Event`, `Board`, `Domain`) declare none of them and stay freely copyable.
- **Destructors never throw.** A destructor is implicitly `noexcept`, so a throw from one calls `std::terminate`.
  Use the non-throwing overload of anything fallible — `std::filesystem::remove(path, errorCode)`, not `std::filesystem::remove(path)`.
- **A pimpl owner declares its destructor.** A class holding a `unique_ptr` to an incomplete type declares `~Thing();` and defines it in the `.cpp`, where the pointee is complete — see `World`.
- **Files and streams are scoped objects.** Construct an `std::ifstream` / `std::ofstream` where it's needed and let scope close it; never an `.open()` / `.close()` pair.

## Error handling

Throw one specific, named exception type per failure category (`ReplayFormatError`, `BoardFormatError`, ...), deriving from `std::runtime_error` and forwarding its constructors — never a bare `std::runtime_error` or a generic exception at the call site.
Document `@throws` on any public method that can throw.

```cpp
class ReplayFormatError final : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
```

## Testing

- GoogleTest, registered with CTest.
- One test file per class under test: `<ClassName>Test.cpp`.
- Test naming: `TEST(<ClassName>Test, <Method>_<ExpectedBehavior>)`, e.g. `TEST(GameStateReducerTest, Handle_AddsToScoreOnCustomScoreIncrementEvent)`.
- Every mock/fake header under a `tests/{mocks,fakes}/include` directory must be `#include`d by at least one `.cpp` file — an unused test double is dead code.
  Enforced by `scripts/check_unused_test_doubles.py` in CI.
- GMock doubles use `MOCK_METHOD(ReturnType, name, (params), (qualifiers, override))`.

## Application entry points

Every `src/apps/<name>/src/main.cpp` is excluded from the coverage report, and it has to earn that exclusion by holding nothing worth covering.
The exclusion is the *reason* for the rule, so the two are always read together: a `main` is the one file CI does not measure, so it must be small enough that nothing can go wrong in it.

- **Branchless.** No `if`, `for`, `while`, `switch`, ternary or `try`/`catch` of its own — an untaken branch in an unmeasured file is exactly the gap the gate exists to catch, and here nobody would see it.
- **One-directional.** Construct, wire, run, return: each statement either builds a collaborator or hands it to the next one, and nothing flows back up.
- **Minimal.** No argument parsing, no validation, no defaulting, no error handling, no message a test would ever want to assert on.
- Anything worth covering moves behind a seam the gate *does* see — a parser, a factory, a runner, a sink — and is tested there like any other class.
- When a `main` grows something a test would want to reach, the fix is to move that thing out, never to relax the gate.

The gcovr `--exclude '.*/apps/[^/]+/src/main\.cpp'` in [`.github/workflows/build.yml`](../.github/workflows/build.yml) is where this is spelled out to CI.

`antwika::app`'s `runRecorded()` is what makes the rule reachable for a tick-loop app: it owns the argument parsing, the try/catch and the record epilogue every `main` used to repeat, leaving construction and one call.

**`src/apps/sudoku/src/main.cpp` is a deliberate exception, and the only one.**
It has no tick loop and no replay, so `runRecorded()` does not apply to it at all, and its branches are its own `--puzzle` handling and its solve-outcome reporting.
It was reviewed and left as it stands rather than given a runner of its own, since a second runner would exist for a single caller.
A new app follows the rule rather than this exception.

## CMake

- One `CMakeLists.txt` per module, building a target `antwika_<module>`, aliased to `antwika::<module>`.
- The public include directory is exposed via `$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>` and `$<INSTALL_INTERFACE:include>`.
- Link against the `antwika::<other-module>` alias, never the raw target name.
- Tests are only added `if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)`.
- Install rules (`ARCHIVE`/`LIBRARY`/`RUNTIME` destinations, the header directory, and an export set namespaced `antwika::`) are mirrored across every module.
- `antwika_add_library(NAME <module> SOURCES ... DEPENDS antwika::<other>)` from `cmake/AntwikaModule.cmake` writes all of the above, and a module with nothing unusual to say should use it -- `src/libs/time/CMakeLists.txt` is the worked example.
  A module that genuinely differs still spells its own rules out, so the difference stays visible.
- `antwika_bundle_app(TARGET <target> [ASSETS <path>...])` from the same file is an application's counterpart, and every application under `src/apps/` calls it.
  It gives the executable a directory of its own under `bin/`, copies the named assets in beside it, and on MinGW copies the runtime DLLs in too.
  An asset is found at run time with `antwika::app::assetPath()`, never by a path baked in at configure time: that path is the building machine's, and a cross build's building machine is never the one that runs the result.
- `antwika_bundle_test(TARGET <target>)` from the same file ends every `tests/CMakeLists.txt`, in place of `include(GoogleTest)` and `gtest_discover_tests()`.
  It puts the suite in the directory of the module that owns it -- the target's own name with the trailing `_tests` taken off -- and registers its cases with CTest, so an application's suite sits beside the executable and a library's gets a directory of the library's name.
  A test target's name therefore has to end in `_tests`, which is what says which module owns it.

## Python (checker scripts)

- Python 3, fully type-hinted, built around `argparse` with a `--root` override defaulting to the repository root.
- A short module-level comment at the top states the script's purpose
  (still one sentence per line).
- Exit `0` and print an `OK: ...` message on success; exit `1` and print each violation with a file/line reference otherwise, so the script composes as a CI pass/fail gate.

## Commits and releases

Commits follow [Conventional Commits](https://www.conventionalcommits.org/)
(`feat(scope): ...`, `fix(scope): ...`, `docs(scope): ...`, `test(scope): ...`,
`perf(scope): ...`, `style(scope): ...`, `chore(release): ...`), scoped to the module(s) touched.
Releases are cut by `semantic-release` from that history — don't hand-edit `CHANGELOG.md` or bump a version manually.

## Enforcement

CI backs the rules above that are automatable:

- Compiler warnings as errors (`-Wall -Wextra -Wpedantic -Wsuggest-override -Werror`).
- `scripts/check_line_length.py` — the 80-character limit.
- `scripts/check_one_sentence_per_line.py` — one sentence per comment/prose line.
- `scripts/check_unused_test_doubles.py` — no dead mocks/fakes.
- `ctest` across all three toolchains (GNU, LLVM, MinGW).
- Coverage instrumentation and reporting for GNU and LLVM (see the [Coverage](../README.md#coverage) section of the README, and
  [`docs/confirming-unreachable-branches.md`](confirming-unreachable-branches.md)
  for when a gap may be excluded instead of tested).
  Each app's `main.cpp` is the one file left out of that report, on the terms set out under [Application entry points](#application-entry-points).

Everything else in this guide is convention enforced by review, not by a script — treat a deviation the same way you'd treat any other review comment.
