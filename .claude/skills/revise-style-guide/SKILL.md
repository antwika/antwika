---
name: revise-style-guide
description: Extend or amend docs/STYLE_GUIDE.md by asking the user multi-option questions with concrete code previews, writing approved answers into the guide, and enforcing what is checkable in scripts/check_comment_style.py. Use when the user wants to add, change, or revisit a comment/documentation style rule, or says "continue" on a style-guide session.
---

# Revising the style guide

The guide is built one decision at a time.
Every rule in it traces to an option the user approved explicitly.
Nothing goes in because it is conventional, and nothing goes in because you inferred it.

## The loop

One round is: ask, act, verify, diff.

### 1. Ask

Ask two or three questions per round with `AskUserQuestion`.
More than three in one round makes the previews too shallow to judge.

Each question gets:

- **Two or three real style options**, not a preference and a strawman. Each must be defensible.
- **A `preview` on every option** showing the actual code that results. This is what the user is choosing between, so it carries the weight. Use their real identifiers and real file paths, not `foo`.
- **A "Neither" option**, always. The user rejecting both framings is a normal outcome and a useful signal.
- **The cost stated in the description.** If an option implies renaming 2477 tests or editing 1900 files, say the number.

Prefer questions whose answers change what you do. Skip anything with an obvious default.

When the user picks "Neither", do not retry the same question with the same axis.
Find a different way to cut the problem and ask again next round.

When the user asks for clarification instead of answering ("which generated code?"), answer it concretely with file paths and counts from the tree, then re-ask with the specifics filled in.

### 2. Act

Write approved rules into `docs/STYLE_GUIDE.md` immediately, in the same turn.

- One `##` section per rule.
- One sentence per line. `scripts/check_one_sentence_per_line.py` gates `docs/**/*.md`.
- Show a rejected example and a required example in a fenced `cpp` block. The contrast is what makes a rule usable.
- Say why the rule exists in one or two sentences, in the guide's voice: declarative, no hedging.

Then enforce it if it is mechanically checkable:

- Add the check to `scripts/check_comment_style.py`.
- Add tests to `scripts/tests/test_check_comment_style.py`, including the boundary case and the false positives the check could produce.
- The gate is already wired into `.github/workflows/build.yml`; a new rule needs no new CI step.

If the tree does not yet satisfy the new rule, add the rule string to `MIGRATING_RULES`.
A migrating rule is counted and reported on every run but does not fail the build, and every other rule keeps failing normally.
Never reach for the global `--warn-only` flag to land a single rule; it downgrades everything at once.

### 3. Verify

Run the gates after every round:

```sh
for s in check_one_sentence_per_line check_line_length \
         check_unused_test_doubles check_comment_style; do
    python3 scripts/$s.py
done
python3 scripts/tests/test_check_comment_style.py
```

Anything touching CMake, C++ or the build needs a real build before it is claimed to work:

```sh
export CONAN_PROFILE=gcc-linux-x86_64
scripts/build.sh
```

### 4. Diff

Snapshot the guide each round and show the user a real diff.

```sh
SNAP=/tmp/style-guide-snapshots
mkdir -p $SNAP
diff -u $SNAP/prev.md docs/STYLE_GUIDE.md
cp docs/STYLE_GUIDE.md $SNAP/prev.md
```

The user may not be able to read files.
Paste the diff into the chat, and paste the full guide when asked.

## Migrating a rule across the tree

A rule the tree does not yet satisfy goes into `MIGRATING_RULES` and comes out again when the count reaches zero.
All three migrations run so far were closed rather than left standing; do not open a fourth and abandon it.

### Open it

Add the rule string to `MIGRATING_RULES` in `scripts/check_comment_style.py`.
The gate then counts and reports it on every run without failing, while every other rule keeps failing normally.
Never reach for the global `--warn-only` flag to land one rule; it downgrades everything at once.

Write the current count into the guide so the debt is visible in the document, not only in the tool.

### Work it

Ask first whether the change is mechanical or needs judgement, because the two are not the same job.

Include order was mechanical: one pass reordered 373 files, and the rule was retired the same day.
Test-name grammar was not: 2486 names, each needing its body read, because the method half of `Size_IsTheSumOfItsEntries` is not recoverable from `AWavesSizeIsTheSumOfItsEntries`.

For the judgement kind, work smallest module first and commit per module or small group.
Each batch is: propose, decide, apply, wrap, build, run, commit.

```sh
S=.claude/skills/revise-style-guide/tools
python3 $S/propose_names.py src/libs/ui
# decide the names, then apply them with rename_tests.py
python3 $S/wrap_test_lines.py src backends
python3 scripts/check_line_length.py
python3 scripts/check_comment_style.py
cmake --build build --target antwika_ui_tests -j24
xvfb-run -a ctest --test-dir build -R 'Dropdown|TextArea' --no-tests=error
```

`propose_names.py` ranks the calls each test body makes, which picks the prefix correctly often enough to be worth checking rather than typing.
Where a whole suite drives one entry point the prefix is uniform and the work is nearly mechanical; `ui` went 164 names that way.

### Close it

Take the entry out of `MIGRATING_RULES`, update the guide from "not yet true" to what is now true, and verify from a clean tree with both the release build and the coverage gate.

### What went wrong last time

**A count you did not measure is wrong.** "418 names" became 575 under a real check, and the difference changed what the user chose to do.

**Never truncate a name to fit.** A cut name collides silently with another in the same fixture, and the collision surfaces as a link error long after.

**A fixture name is not unique across the repository.** Seven modules define `StateDumpTest`, each in its own binary, so a global uniqueness check reports collisions that are not real.

**Print the whole proposal.** Piping through `head` hid the tails of long files and six modules were reported clean while still holding names.

**A longer name is a longer line.** Three batches tripped the eighty-column gate before wrapping was scripted.

**Finish the sweep you started.** Renaming to `Equality_` and `Construction_` satisfied the checker while breaking the rule's actual wording, and cost 395 further renames to undo.

## Rules for you, not for the guide

**Check every factual claim against the tree before it goes in a question or the guide.**
Counts, conventions and "this is already how it works" are the claims that turn out wrong.
Run the grep. A wrong number in an option changes the user's decision.

**When you get a number wrong, correct it plainly and adjust the guide.**
It happened with test-name grammar: the claim was "already the convention in ~5900 tests", the reality was 3045 of 5517. The guide now records the real figure.

**Surface contradictions the moment a new rule creates one.**
Approving a rule often invalidates an example already in the guide. Say so and fix it in the same turn.
A terseness rule and a seventy-five-character allowance cannot both be silent about test names.

**Check a new rule against the rules already there.**
A limit that another gate makes unreachable is decoration. A 120-character name cannot exist under an 80-column line limit, because a C++ identifier cannot wrap.

**Never widen scope silently.**
Scope decisions are the user's: which languages, which directories, whether generated output counts.
`build-*` and `.claude/` are out of scope; the tracked source tree is in.

**Watch for the string-literal trap.**
A comment checker or stripper that greps will destroy `#include` lines inside a multi-line CMake string, `${VAR#...}` in shell, `//` in a URL, and `#` in a Python string.
Parse, do not grep. This has already broken a build once.

## State

- The guide: `docs/STYLE_GUIDE.md`
- The gate: `scripts/check_comment_style.py`, tests alongside in `scripts/tests/`
- No open migration; `MIGRATING_RULES` is empty
- Three migrations have closed: include order, test-name grammar and streamed prose
