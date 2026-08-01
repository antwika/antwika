# One table, one parse, and no dependencies

`antwika::cli` is the whole of this project's command-line handling: a `FlagSpec` table, `parseCommandLine()`, `helpText()`, a `CommandLine` to ask questions of, and `CommandLineError` for a command line that was refused.
It depends on nothing -- not `antwika::replay`, not `antwika::log`, not `antwika::time` -- and that is the point of it existing as a library rather than as part of the one that used it first.

It began inside `antwika::replay`, next to `ReplayCli`, because `--record` and `--replay` were the only flags anything took.
Then `apps/game` grew `--save`/`--load` and `apps/poker` grew `--tick-delay-ms`, and each of them had to link a JSON replay format, a schema validator and a migration chain in order to read two dashes and a word.
`ReplayCli` stayed behind, because naming `--record` and `--replay` really is replay's business; the machinery that reads them is not.

## The table is the parser and the help text

A `FlagSpec` is a name, a value name and a line of help, and the same `std::span<const FlagSpec>` is passed to `parseCommandLine()` and to `helpText()`.
So a flag that parses but is undocumented, or is documented but refused, is not expressible: there is one list, and both answers are derived from it.
`valueName` doing double duty is what makes that hold -- an empty one means the flag takes no value, which is simultaneously how the help text renders it and how the parser knows not to swallow the next argument.

`--help` is not in any caller's table.
It is `kHelpFlag`, declared by this library, accepted by every parse and appended to every help text, so no program can forget it and no program can document a `--help` it does not accept.

## Parsing happens exactly once

`parseCommandLine()` throws `CommandLineError` on an argument that is not in the table.
That is what makes the second rule necessary: a program with flags of its own concatenates its table with everybody else's and parses the result **once**.
Parsing twice -- once for the shared flags, once for the app's -- means each pass refuses what the other accepts, and that is not hypothetical: it is how `apps/poker`'s `--tick-delay-ms` stopped working.

This is why the library offers tables and not parsers.
`replayCliFlags()`, `game::saveCliFlags()` and `poker::watchFlags()` are all tables handed to `app::runRecorded()`, which appends them, parses once, and hands the resulting `CommandLine` back for each of them to read its own flags out of with `replayCliOptionsFrom()`, `saveCliOptionsFrom()` and `watchOptionsFrom()`.
A `<Something>Options` struct read out of an already-parsed `CommandLine` is the shape to copy for a new flag.

## Refusing is the feature

Both refusals -- an unknown argument and a flag left without its value -- used to be silently ignored.
That is how `--replya demo.json` started an empty session instead of saying it was a typo, and the run looked completely normal while doing nothing that was asked of it.
`CommandLineError` is its own type rather than a `bool` or a `std::runtime_error` because the two cases read identically at a call site, and because a program refusing its arguments is a different thing from a program failing at its work.

A repeated flag keeps its last value, deliberately.
A command line is usually the last line of a shell history edited one flag at a time, and the value nearest the end is the one just typed.

## Flags only, and no positional arguments

There are deliberately no bare arguments: everything a program is told arrives as a named flag.
A positional would have to be described somewhere the table cannot reach, so the one property this library is built on -- that the parse and the help text come off the same list -- would hold for flags and quietly not for the rest.

`apps/sound_demo` is what settled it.
It took a bare filename, so it was the one program in the tree that parsed its own `argv` instead of using this library, and it paid both of the prices refusing is meant to prevent: `--help` did nothing, and `--flie my.wav` was taken for a filename and failed much later inside the WAV reader rather than at the command line.
It names `--file <path>` now, and there is no program left here that reads its own arguments.

## What is left in `antwika::replay`

`ReplayCli` and nothing else of the command line.
`antwika/replay/CommandLine.hpp`, `FlagSpec.hpp` and `CommandLineError.hpp` were `using` re-exports of the `antwika::cli` names under the old spelling, kept while callers still wrote `antwika::replay::FlagSpec`; every caller has moved, so the three headers are gone.
`replayCliFlags()` and `replayCliOptionsFrom()` name `cli::FlagSpec` and `cli::CommandLine` directly, which is the only spelling there is.
