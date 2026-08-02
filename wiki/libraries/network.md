# network

`antwika::network` carries bytes between hosts, through a build-time backend seam exactly as [`gfx`](gfx.md), [`input`](input.md) and [`sound`](sound.md) have — so no code under `src/` names a socket API.

It is the **transport half** of multiplayer and deliberately nothing more.
It depends on [`log`](log.md) and nothing else: not `event`, not `time`, not `simulation`.
That is the decision the rest follows from, and it is structural rather than a promise — this library cannot name a `Tick` or an `Event`, so it cannot smuggle a socket fact into simulation state even by accident.

## What it demonstrates

- A fourth subsystem behind the same backend seam the other three use.
- A transport that owns **no thread, no lock and no queue**.
- Real two-peer networking as an ordinary unit test, with scripted latency and loss.

## The rule the whole design hangs on

> The network reaches a simulation only as `event::Event`s supplied by an `ITickEventSource` for a specific tick, and never in any other way.

Remote input *is* input, in exactly the sense a click is: external, unpredictable, and arriving from outside the process — which is what `ITickEventSource` already models and what a `--record` file already holds.
A networked session that is recorded therefore replays **single-player, with no socket opened**, because the remote events were captured on the way in like every local one.

Three corollaries, each of which rules out a whole family of designs:

- **No wall clock may decide what is computed.** Latency and arrival order may decide what is *drawn* — `game::FrameMeter`'s situation exactly — and nothing else. Timeouts are counted in ticks.
- **A peer dropping is a decision every peer must reach identically.** Each one timing out on its own clock is a divergence with the symptom nowhere near the cause.
- **No thread.** This codebase has no concurrency model, and a socket thread would be the second one.

## Owning no thread

A host does nothing until `pump()` asks it to, on the thread that asked — `sound::IDevice`'s decision, taken for `sound`'s reasons.

```cpp
host->send(peer, payload);   // queued, not sent
other->pump();               // now it moves
const auto arrived = other->receive();
```

`pump()` and `receive()` are separate so a caller can pump repeatedly while waiting on a slow peer without re-walking a queue, and so `receive()` stays a `[[nodiscard]]` query.
A packet is handed over once and forgotten.

## Interfaces

| Type | What it is |
| --- | --- |
| `PeerId`, `Port` | Scoped enums with `rawValue()`, per the strongly-typed-id idiom |
| `Endpoint` | A host name and a `Port`; ordered, so a registry can key on it |
| `Packet` | A `PeerId` and the bytes, and nothing shaped |
| `NetworkCapabilities` | `connects`, `listens`, `maxPeers`, `maxPayloadBytes` |
| `IHost` | One open endpoint: `connect`/`disconnect`/`send`/`broadcast`/`pump`/`receive`/`peers` |
| `INetworkBackend` | `name()`, `capabilities()`, `openHost()` |
| `NetworkError` | The one failure category |

**A peer's name is local to the host that assigned it.**
The id host A holds for host B is not the id B holds for A, and neither travels anywhere — which is exactly how a socket behaves.
An identity a whole session agrees on is something peers reach by talking, so it belongs to the layer above rather than to a transport that has only ever seen one end of a link.

**A peer that has gone is not an error.**
`send()` to an id this host does not hold is a no-op, because a link dropping between reading `peers()` and sending is what a network does, and there is no better answer a caller could give.
`NetworkError` is left for genuine refusals: an oversized payload, a host already open at an endpoint, a backend asked to connect when it says it cannot.

**Nobody being there is not an error either**, and that is the one place saying so costs something worth paying for.
A non-blocking connect over a real network cannot know whether anything is listening until several pumps later, so a backend that refused an unreachable endpoint in `connect()` could only do it by blocking — inside a tick, on the thread running the simulation.
The answer arrives the way a dropped link does instead: the peer is not in `peers()` until it is up, and one that never comes up simply never appears there.
`LoopbackBackend` *could* answer at once and deliberately does not, because a contract two backends keep differently is one a caller cannot rely on.

**A host reports where it is, not what it was asked for.**
A backend handed port 0 is being asked to pick one, so `IHost::endpoint()` is the authority and the argument `openHost()` took is only a request.
That is what lets the conformance suite open every host on an ephemeral port and dial whatever each reports — which is the only workable thing for a real socket, and the same thing for the in-process backends.

**`pump()` is where bytes move, in both directions.**
`send()` queues; the bytes leave on the *sender's* next pump.
A real socket takes as much as it feels like and leaves the rest, so a send that wrote its payload out on the spot would either block or fail halfway.
`LoopbackBackend` defers too, rather than handing the packet straight over — deliberately, so a test that passes against it passes against a socket.

## Two backends live in the library

`NullNetworkBackend` and `LoopbackBackend` are under `src/libs/network/` rather than under `backends/`, following the `NullSoundBackend` and `NullInputBackend` precedent: `backends/` is exempt from the coverage gate and `src/` is not, so an in-process implementation kept here is one the gate can hold to 100%.
All that lives under `backends/null/` is the two-line factory.

**`NullNetworkBackend`** talks to nobody.
Holding no peers is an ordinary state rather than a failure, so sending, broadcasting, pumping and disconnecting all do nothing and none of them is refused — which is what lets a single-player build link the seam for the price of a virtual call a tick.
The one thing it *does* refuse is `connect()`, because `capabilities().connects` is false and a backend that accepted a dial it could never complete would be no use as a stand-in for one that can.

**`LoopbackBackend`** is what makes real multiplayer a unit test: every host in one process, wired to each other, with no socket, no port and no wall-clock time spent.

```cpp
LoopbackBackend network(logger, DeliverySchedule{.delayPumps = 2,
                                                 .dropped = {1, 7}});
const auto left = network.openHost(here);
const auto right = network.openHost(there);
const PeerId peer = left->connect(there);
```

**Latency and loss are scripted rather than sampled**, which is the whole point: a test that drops the second packet drops the second packet on every machine and every toolchain, where one drawing from a generator would be a different test every run and a different one again under instrumentation.
`DeliverySchedule::delayPumps` is counted on the *receiving* host, and `dropped` names sends by their ordinal across the whole network — so "the third packet" means the third one anybody sent, and a broadcast spends one ordinal per peer it reaches.

It is deliberately **not** selectable as `ANTWIKA_NETWORK_BACKEND`, since a build asking for a transport wants one that leaves the process.

## Choosing a backend

`-o network_backend=` and the `ANTWIKA_NETWORK_BACKEND` CMake variable, defaulting to `null`.

It does **not** follow `gfx_backend` the way input does, for `sound_backend`'s reason rather than a new one: a build asking for sdl3 windows has said nothing whatever about wanting to open a socket.

## Non-obvious decisions

**A link stores both of its names.**
Each `Link` holds this host's name for the peer *and* the peer's name for this host, so delivering a packet is never a search for who sent it.
The alternative — the receiving host looking the sender up by pointer — is the same answer reached more slowly, and it has a "not found" arm that could never be taken.

**A host may not outlive the backend that opened it.**
Hosts find each other through state the backend owns, so the usual rule about a borrowed collaborator applies to the thing that handed the borrow out.

**Packets already in flight outlive their sender leaving**, exactly as on a real network: what is in flight is in flight, and a host closing takes itself off every peer without recalling what it has already sent.

## Conformance

`antwika::network::tests::conformance` is the shared suite, run against all three backends.
Backends under `backends/` cannot be held to the coverage gate — CI has no network to reach — and this is what replaces that: a backend is finished when it passes the suite unmodified.

**A backend that cannot do something skips rather than fails**, and `NetworkCapabilities` is how it says so.
The alternative — a backend pretending to connect so a test goes green — is exactly the dishonesty a conformance suite exists to prevent.

Nothing in the suite assumes delivery is immediate.
Every wait is a bounded run of pumps at *both* ends, so a transport that takes a few of them to settle passes the same tests an in-process one does.

**What arrived before a peer left is still owed to the caller**, which is `Send_ThenDisconnectStillArrives`.
Send-then-disconnect is one message on a stream transport: the payload and the close land in the receiver's buffer together, so a backend that reacted to the close first dropped a frame that had already arrived whole — which is exactly what `SocketsHost` did until the suite grew a case for it.
Stating it in the suite rather than fixing it in the one backend is the point: `LoopbackHost` is held to the same rule, so a caller can rely on it whichever transport a build selected.

**Writing the second backend is what found the first one's contract gaps**, which is the argument for the suite rather than a story about it.
Three things came out of it: `connect()` refusing an unreachable endpoint was a promise only an in-process backend could keep; `endpoint()` returning the requested port was a promise only a backend that never binds could keep; and the suite pumping one end of a link had been passing because `LoopbackBackend` delivered on `send()` where a socket cannot.
Each was fixed in the contract rather than papered over with a capability flag, since a capability should say what a backend *can do* rather than which of two behaviours it has.

## The sockets backend

`backends/sockets/` is the one that leaves the process: non-blocking TCP, IPv4, chosen with `-o network_backend=sockets`.

**It is the first backend in this tree that is not a framework**, and that is why it is exempt from the one-real-framework rule the other three subsystems obey.
Both reasons that rule gives are about a framework — a process-global event queue, and a doubled dependency graph — and a backend naming the operating system's own socket API has neither.
So `-o gfx_backend=sdl3 -o network_backend=sockets` is an ordinary configuration, it resolves against `conan-sdl3.lock` unchanged, and it adds no package at all.
The exemption is written down in both places that enforce the rule, `conanfile.py` and `backends/CMakeLists.txt`.

**TCP rather than UDP**, because the layer above is lockstep: it needs every peer's input for a tick, in order, and rebuilding reliability over datagrams would be a second transport written inside a backend.
What TCP costs in exchange is message boundaries, so a payload travels behind a four-byte **big-endian** length — big-endian because a wire is one place a byte order has to be stated rather than inherited from whichever machine wrote it.

`src/SocketApi.hpp` is the one file that names either platform's socket API.
The two operating systems disagree about the handle type, how one is closed, how one is made non-blocking, and where the last error lives — and about nothing else this backend does, so everything above that file is one code path.
Winsock's process-wide start-up lives there too, as a function-local static, following `backends/sdl3`'s runtime archive: a framework directory owns that framework's global state.

Nothing here blocks. Every socket is non-blocking and `pump()` polls with a zero timeout, because a tick may not sleep.

## What is not here yet

The tick-domain layer above this one — the wire codec, the per-peer input buffers and the `ITickEventSource` decorator that turns arriving bytes into a tick's events.
That layer is a separate library on purpose, so that a single-player build links this and no wire format at all.

## See also

- [`sound`](sound.md) — where the pumped, thread-free arrangement comes from
- [`input`](input.md) — where the "what lands in a recording is decided by where the recorder sits" rule is written down
- [`simulation`](simulation.md) — the `ITickEventSource` seam remote input will arrive through
