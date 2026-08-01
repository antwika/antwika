# antwika::log

`src/libs/log/` — composable logging with no global state.

## What it is for

Emitting diagnostic messages, with the decisions about *whether*, *how* and *where* separated into three injected pieces.
It is used across the applications and carries no tick or replay logic of its own, exactly as [`time`](time.md) does.

## Key types

| Header | Type | Role |
| --- | --- | --- |
| `ILogger.hpp` | `ILogger` | What everything else in the project takes a reference to. |
| `Logger.hpp` | `Logger` | Composes a policy, a formatter, an appender and an `time::IClock`. |
| `Level.hpp` | `Level` | The severity levels. |
| `ILogPolicy.hpp` | `ILogPolicy` | Decides whether a message is emitted at all. |
| `MinimumLevelLogPolicy.hpp` | `MinimumLevelLogPolicy` | The usual policy: emit at or above a level. |
| `IFormatter.hpp` | `IFormatter` | Turns a message into text. |
| `PlainFormatter.hpp` | `PlainFormatter` | The default text form. |
| `IAppender.hpp` | `IAppender` | Where the text goes. |
| `StreamAppender.hpp` | `StreamAppender` | To a `std::ostream`. |
| `NullAppender.hpp` | `NullAppender` | Nowhere, which is what tests and headless runs use. |

`MockLogger`, `MockLogPolicy`, `MockFormatter` and `MockAppender` live under `tests/mocks/`.

## Depends on

[`time`](time.md), for the `IClock` a timestamp comes from.

## Non-obvious decisions

**No singleton, no global logger, no macros.**
An `ILogger &` is passed to whatever needs one, which is what makes a logging assertion an ordinary expectation on a mock rather than a scrape of captured output.
It also means two parts of one process can log differently without fighting over a global.

**Three interfaces where one class would do.**
Policy, formatter and appender are separate because each is a different reason to change, and each can be unit-tested against a mock in isolation.
The project keeps an interface with a single implementation when that is what it buys — see [`docs/STYLE_GUIDE.md`](../../docs/STYLE_GUIDE.md).

**Time comes from `IClock`, not from the appender.**
A timestamp is part of the message, so it is decided where the message is built, with an injected clock a test can freeze.
