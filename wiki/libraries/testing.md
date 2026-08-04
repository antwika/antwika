# antwika::testing

`src/libs/testing/` — the scratch paths every suite's temporary files need, named so two processes cannot collide.

## What it is for

One thing: naming, making and removing a temporary file or directory for the case that is running.

It exists because fifteen test files had written that out for themselves -- twelve inline `ScratchFile`/`TempFile` classes, two shared headers, one `scratchDirectory()` -- and the naming rule they all depended on was a comment those copies cited rather than shared.
That rule is load-bearing rather than tidy: `SessionPersistenceTest` flaked at 22 failures in 160 paired runs because one fixed path was shared by two processes, and the fix was to put the pid in the name everywhere.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ScratchPath.hpp` | `scratchPath()` | A path under the temporary directory carrying the case name and the pid. |
| `ScratchPath.hpp` | `ScratchFile` | That path as a file, removed on scope exit, with `write()`/`path()`/`string()`. |
| `ScratchPath.hpp` | `ScratchDirectory` | The directory counterpart, `remove_all`'d on the way out, whose `pathIn()` names a file inside it. |

## Depends on

GoogleTest, for the current case's name.
Nothing else.

## Non-obvious decisions

**The pid is what stops a re-run colliding with itself**, and the case name is what stops two cases of one fixture colliding.
CTest registers every case as its own process, so a path named for the fixture alone is shared by every case of it running at once, and a fixture that clears the path on construction wipes a neighbour's files mid-run.
A path that has never existed before cannot be caught halfway through being removed, which an overlay filesystem under load will otherwise do.

**Removal happens on scope exit rather than at the end of a test body**, so a *failing* assertion leaves no debris behind.

**It is a library rather than a header copied into each module's `tests/`**, on the same terms `antwika::i18n`'s conformance suite is: the rule has to be right once, and a copy is a rule that can drift.
