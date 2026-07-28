# Writing the rules down: prose linting, doc comments, and a requirements file

*2026-07-28*

The [previous post](2026-07-27-building-a-deterministic-replay-system.md) was about a feature — a deterministic replay system, with a bug a test caught before it shipped.
This one isn't about a feature at all.
It's about the day after: turning a handful of conventions that had only ever lived in people's heads (and in review comments) into checks CI actually runs, comments the public API actually carries, and a single file that says what the whole project is for.

Four commits, two pull requests, one follow-up file: one sentence per line, a hard 80-character line limit, Doxygen comments on every public interface, and `REQUIREMENTS.md`.
None of them touch runtime behavior — `antwika_game` does exactly what it did before.
What changed is how much of "what this project expects of its own code" is now enforced by a script instead of remembered by a person.

## One sentence per line

The convention already existed informally: comments and markdown prose, one sentence per line, however long that line gets.
It's a good rule for diffs — a one-word edit inside a paragraph shows up as a one-line change instead of a reflowed wall of text — but an unenforced convention is a convention that erodes the first time someone's in a hurry.

`scripts/check_one_sentence_per_line.py` makes it real.
It walks `README.md`, `blog/*.md`, every `//` comment under `src/**/*.{cpp,hpp}`, and every `#` comment under `scripts/*.py`, and fails on two distinct violations:

- **Multiple sentences sharing a line** — detected with a regex that looks for `[.!?]` followed by whitespace and a capital letter, quote, or backtick, after masking out anything that would produce a false positive: code spans (`` `like.this` ``) and abbreviations (`e.g.`, `i.e.`, `etc.`, `vs.`, `cf.`, `approx.`).
- **One sentence wrapped across multiple lines** — detected by watching for a run of consecutive comment lines (a "chain") where every line but the last doesn't end in sentence-ending punctuation, meaning it must continue on the next line.

Getting the false-positive rate down took more of the script's logic than the happy path did.
Markdown lines that are pure badges or link references — nothing but `[...]`/`(...)` groups — needed to be recognized as "not prose" so a README badge row wouldn't trip a violation for having no sentence-ending punctuation at all:

```python
def is_pure_markup(text: str) -> bool:
    # Drops the *contents* of every (...)/[...] group -- link/image labels and
    # URLs alike -- so a line that's nothing but badges/links has no letters left.
    residual = []
    depth = 0
    for char in text:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth = max(depth - 1, 0)
        elif depth == 0:
            residual.append(char)
    return not re.search(r"[A-Za-z]", "".join(residual))
```

Once the checker existed, it immediately found real violations — mostly in the *previous* blog post and in `README.md`, both written before the rule was enforced.
Reflowing those was most of the diff: multi-line paragraphs collapsed to one sentence per line, most visibly in the replay-system post, which went from readable-in-a-narrow-terminal prose to long single-sentence lines.
The C++ and Python comment bases needed smaller fixes — a few `//` comments that had wrapped a sentence across two lines got joined back onto one.

The script wired into CI right next to the existing `check_unused_test_doubles.py` step, and got its own test suite (`test_check_one_sentence_per_line.py`) before it started gating merges — the same pattern the project already used for its other checker script, not a new one invented for this.

## An 80-character hard limit

The one-sentence rule constrains *prose*; it says nothing about code.
A follow-up commit added a second, much simpler checker: `check_line_length.py` fails if any line in `src/**/*.{cpp,hpp}` or `scripts/*.py` (and its own test file) exceeds 80 characters.
No masking, no regex — just `len(raw) > 80`, reported with the actual length so it's obvious how far over a line is:

```python
def find_long_lines(path: Path) -> list[tuple[int, int]]:
    violations = []
    lines = path.read_text(errors="ignore").splitlines()
    for line_no, raw in enumerate(lines, start=1):
        if len(raw) > MAX_LINE_LENGTH:
            violations.append((line_no, len(raw)))
    return violations
```

The commit's own description of what came next is accurate: "wrap every C++ and Python source line over the limit to comply."
That touched 59 files, almost all one-line diffs each — a designated-initializer struct broken across several lines, a long `bootstrap(...)` call split one argument per line, a thrown `ReplayFormatError` message split into adjacent string literals, a shift computation given its own line.
None of it changed behavior; all of it was reflow.
`BinaryPrimitives.cpp` picked up one small side effect worth naming: shortening an over-long comment (*"...below are thin, named wrappers over this"* → *"...below are thin, named wrappers"*) turned out to be easier than wrapping it, which is itself a small piece of evidence that a hard line limit nudges toward tighter writing, not just more line breaks.

Same rollout pattern as the sentence checker: a test file first, then the checker itself, then a CI step — in that order, so the gate that starts blocking merges is never the first version of itself to run.

## Doxygen comments, and deciding they're not "comments" for rule purposes

Public interfaces — `IEventDispatcher`, `ILogger`, `IEngine`, `IReplaySource`, `EngineLoop`, and the rest — picked up `@brief`/`@param`/`@return` blocks:

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

This is where `EngineLoop`'s six-line narrative comment about *why* the loop body is identical for live and replayed runs — the one thing the previous post spent a whole section justifying — turned into a `@brief` paragraph instead of a run of `//` lines, unchanged in substance.

It also created a real tension with the project's existing "comments should default to absent" preference, which exists specifically to keep comments meaningful rather than decorative.
Doxygen blocks are decorative in exactly the sense that rule warns against — `@param event The event to dispatch` says nothing a reader of `dispatch(Event event)` didn't already know.
The resolution, made explicit rather than left as an inconsistency for someone to trip over later, was to carve out a named exception: Doxygen blocks on public API surface (interfaces, classes, public methods) document the *what*, for generated reference docs, and are kept regardless of whether the *why* would otherwise justify a comment.
That exception is written down in `REQUIREMENTS.md`, added in a small follow-up commit after the requirements file's first draft shipped without it.

## Writing `REQUIREMENTS.md`

The last piece pulls back from individual rules to write down the shape of the whole project — not new policy, but the existing one made legible in one place instead of scattered across `README.md`, the replay-system blog post, the CI workflow, and the checker scripts themselves.

It's organized as MoSCoW statements — Must / Should / Could / Won't have — which forces a distinction the source material never had to make explicitly.
"Every mock/fake header must be `#include`d by at least one `.cpp` file" and "loading a replay must deterministically reproduce live state" are both must-haves, but they came from very different places: one from a checker script's exit code, the other from a paragraph in a blog post about a bug.
Flattening both into the same list is only honest if the *won't*-haves get written down with the same weight — so "no RNG in the engine's current scope" sits next to "an index over replay events won't be built until replays are long enough for it to matter," both framed as deliberate, revisitable decisions rather than gaps.

Two lines are worth calling out specifically, because they're the two rules this post is otherwise entirely about:

> Source lines (`src/**/*.cpp`, `src/**/*.hpp`) and script lines (`scripts/*.py`, `scripts/tests/*.py`) must not exceed 80 characters.
>
> Comments and markdown prose must hold exactly one sentence per line, with no sentence wrapped across multiple lines.

Writing them as must-haves next to "the project must be licensed under the Apache License 2.0" is a small statement in itself: these aren't style preferences that happen to be automated, they're requirements the automation exists to satisfy.

## Where it ended up

- Two new checker scripts (`check_one_sentence_per_line.py`, `check_line_length.py`), each with its own test file, each wired into CI the same way the pre-existing `check_unused_test_doubles.py` was.
- 59+ source and script files reflowed to fit an 80-character limit; `README.md` and the previous blog post reflowed to one sentence per line — no behavior changed anywhere in that set.
- Every public interface across `event`, `engine`, `log`, `replay`, and `time` now carries `@brief`/`@param`/`@return` Doxygen comments, with an explicit, written-down exception to the project's own "comments should default to absent" rule to justify why they're kept.
- `REQUIREMENTS.md`: one file, MoSCoW-organized, gathering rules that used to live only in `README.md`, one blog post, a CI workflow, and the checker scripts' source code — including a same-day correction once the Doxygen exception needed a place to live.

The common thread across all four commits is the same move, applied four times at different scales: notice a rule the project was already following informally, then move it somewhere it can't quietly stop being followed — a script CI runs, a comment the compiler-adjacent tooling can see, or a document that says so in plain language.
