# antwika::cli

`src/libs/cli/` — flag tables, one parse, and the help text that comes off the same table.

## What it is for

Reading a program's arguments: what flags it accepts, what was actually typed, and what to print when somebody asks `--help`.

It depends on nothing, and that is why it is a library of its own.
It began inside [`replay`](replay.md), next to `ReplayCli`, back when `--record` and `--replay` were the only flags anything took — so by the time `apps/game` had `--save`/`--load` and `apps/poker` had `--tick-delay-ms`, reading two dashes and a word meant linking a JSON replay format, a schema validator and a migration chain.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `FlagSpec.hpp` | `FlagSpec` | One flag: its name, its value's name in the help text, and a line about what it does. |
| `CommandLine.hpp` | `CommandLine` | What was given, asked with `has()` and `value()`. |
| `CommandLine.hpp` | `parseCommandLine()` | `argc`/`argv` against a `std::span<const FlagSpec>`. |
| `CommandLine.hpp` | `helpText()` | The same table rendered, with `--help` appended. |
| `CommandLine.hpp` | `kHelpFlag` | `--help`, declared here so no table can forget it. |
| `CommandLineError.hpp` | `CommandLineError` | An unknown argument, or a flag left without its value. |

## Depends on

Nothing.

## Non-obvious decisions

**One table is the parser's input and the help text's.**
A flag that parses but is undocumented, or is documented but refused, is not expressible, because both answers come off the same `std::span<const FlagSpec>`.
`FlagSpec::valueName` doing double duty is what holds that together: an empty one is simultaneously how the help text renders the flag and how the parser knows not to swallow the next argument.

**`--help` is not in any caller's table.**
It is accepted by every parse and appended to every help text, so no program can leave it out and none can document a `--help` it does not accept.

**A program parses once, against one concatenated table.**
An argument not in the table is a `CommandLineError`, so a second pass would refuse whatever the first accepted — which is not hypothetical: it is how `apps/poker`'s `--tick-delay-ms` stopped working.
That is why each layer offers a *table* and a reader rather than a parser of its own: `replayCliFlags()`/`replayCliOptionsFrom()`, `game::saveCliFlags()`/`saveCliOptionsFrom()`, `poker::watchFlags()`/`watchOptionsFrom()`, appended and parsed in one place by [`app`](app.md)'s `runRecorded()`.

**Refusing is the feature.**
Both refusals — an unknown argument, and a flag left without its value — used to be silently ignored, which is how `--replya demo.json` started an empty session instead of saying it was a typo.
A run that ignores what it was told looks completely normal while doing none of it.
`CommandLineError` is its own type rather than a `bool` or a `std::runtime_error`, because the two cases read identically at a call site and because a program refusing its arguments is a different thing from a program failing at its work.

**A repeated flag keeps its last value.**
A command line is usually the last line of a shell history edited one flag at a time, so the value nearest the end is the one just typed.

**Flags only — there are no positional arguments.**
A positional would have to be described somewhere the `FlagSpec` table cannot reach, so the property this library is built on — that the parse and the help text come off one list — would hold for flags and quietly not for the rest.
[`apps/sound_demo`](../apps/sound_demo.md) is what settled it: it took a bare filename, so it was the one program in the tree parsing its own `argv`, and it paid both prices refusing is meant to prevent — `--help` did nothing, and `--flie my.wav` was taken for a filename and failed much later inside the WAV reader.
It names `--file <path>` now, and nothing here reads its own arguments.

## What is left in `antwika::replay`

`ReplayCli` and nothing else of the command line.
Naming `--record` and `--replay` really is replay's business; the machinery that reads them is not.
Three `using` re-export headers under `antwika/replay/` carried the old spelling while callers moved, and they are gone — `antwika::cli::` is the only spelling of these types.

## See also

- [`replay`](replay.md) — `ReplayCli`, which names `--record`/`--replay` and parses them with this.
- [`app`](app.md) — `runRecorded()`, the one place the tables are concatenated and parsed.
