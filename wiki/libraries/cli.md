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
Both refusals used to be silent, which is how `--replya demo.json` started an empty session instead of saying it was a typo.
A run that ignores what it was told looks completely normal while doing none of it.

**A repeated flag keeps its last value.**
A command line is usually the last line of a shell history edited one flag at a time, so the value nearest the end is the one just typed.

## See also

- [`docs/cli.md`](../../docs/cli.md) — the long-form argument.
- [`replay`](replay.md) — `ReplayCli`, which names `--record`/`--replay` and parses them with this.
