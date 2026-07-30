# Plan: adding a `ui` library

*Historical: this plan has shipped, and `src/libs/ui/` together with the `antwika::ui` section of `CLAUDE.md` is what supersedes it; it is kept for the reasoning, not as current reference.*

A plan for adding an immediate-mode UI library to Antwika: nestable layouts, buttons and text, drawn through the existing four-call `antwika::gfx::IRenderer` and laid out arithmetically with no help from any graphics framework.

**Status: built.**
All five phases below have landed.
Where the design changed while it was being written, the text says so under [What changed while building it](#what-changed-while-building-it) rather than being quietly rewritten, since the reasoning is the point of keeping this document.

## Context

Nothing in the repository lays out a picture.
Every app that draws does its own integer arithmetic inline:

- `src/apps/gfx_demo/src/DemoScene.cpp` is 52 lines computing bar positions from `canvas.width / kUnitsAcross`.
- `src/apps/poker/src/TableScene.cpp` hand-rolls file-private `scaleFor()`, `lineHeight()`, `cardWidth()`, `seatRowHeight()` and `fillBorder()`, centres text at three separate sites with `(container - textSize(...).width) / 2`, and guards unsigned underflow by hand with `top > headerBottom ? top - headerBottom : 0`.

There is no reusable box, label or centring helper anywhere under `src/`.
Every new panel re-derives the same arithmetic, and every one of them re-derives the same underflow guards.

This library is where that arithmetic goes once.

Scope is deliberately **display only**: layouts, buttons and text as a *picture*.
Nothing in this plan reads a pointer, hit-tests a widget, or sends anything back into the engine.
See [Deferred deliberately](#deferred-deliberately) for why, and for what that leaves for later.

## Goals

- Layouts nest: a row inside a column inside a panel, where an inner size comes from the text it contains and an outer size comes from the canvas.
- Layout is pure arithmetic over `gfx::textSize()` and the fixed-cell font metrics, never a question for a backend.
- Drawing stays a write-only projection, so `REQUIREMENTS.md:40` holds by construction rather than by promise.
- The whole library is unit-testable as values, with no window, no backend and no framework.
- 100% line/function/branch coverage with no new `GCOVR_EXCL_LINE` beyond the two shapes the repo already documents.
- `antwika::ui` depends on `antwika::gfx` and nothing else.

Explicit non-goals: pointer input, hit-testing, interaction state, any `event`/`replay` dependency, and any change to `antwika::gfx` or to `backends/`.

## Decision: deferred immediate mode, in three pure stages

The caller writes immediate-mode code.
What that code builds is a tree, laid out and flattened after the fact:

```
Context (immediate-mode calls)  ->  LayoutTree   flat arena of Node
LayoutTree + canvas             ->  layout()     measure, then arrange, in place
LayoutTree                      ->  flatten()   -> DrawList (a value)
DrawList + IRenderer            ->  paint()      write-only translation
```

### Why not one pass

A one-pass design (microui-style) computes a widget's `Rect` at the moment the call happens, which requires the parent's content rect to be known before its children are declared.
That makes four things impossible:

1. Shrink-wrapping a container around its children's sizes.
2. Distributing leftover space among growing children alongside fixed siblings.
3. Centring a container's content inside it.
4. Nesting any of the above, where the innermost size comes from `gfx::textSize()`.

microui's answer is `mu_layout_row(widths[], height)` -- the caller pre-declares every number.
That does not remove the arithmetic in `TableScene.cpp`, it moves it back to the call site, which is exactly where it already is.
The requirement is that *layouts* nest, not that the caller does, so one pass fails it as stated.

### The property that makes the deferred version cheap

In an immediate-mode build phase a node is always appended after its parent and after its earlier siblings.
So in a flat `std::vector<Node>`, every child's index is greater than its parent's, and siblings appear in ascending index order.

Therefore:

- **measure** is a flat *descending* index loop -- every child is resolved before its parent;
- **arrange** is a flat *ascending* index loop -- every parent is resolved before its children;
- **paint order** is ascending index, which is parent background before child background before text, exactly right for a renderer with no z-order.

No recursion, no explicit stack, no depth limit, O(n) per pass.
It also avoids a class of coverage pain: no recursive function means no exception-unwind landing pad at a closing brace.
This is the same flat, index-addressed shape as `antwika::wfc`'s wave.

### Why `DrawList` is a value

`paint()` is the only thing in the library that touches `IRenderer`, so `MockRenderer` appears in exactly one test file.
Every layout, nesting and sizing assertion becomes a plain `EXPECT_EQ` on a `std::vector`.
Compare `src/apps/poker/tests/TableSceneTest.cpp`, which has to `AnyNumber()`-permit every call because the picture is large and incidental.

It also earns two things that are otherwise unavailable.
`IRenderer` has no scissor, so a child that overflows its parent draws over everything -- layout has to be the clipper, and only a separate stage can guarantee containment.
And because the font is fixed-cell, text can be truncated to whole glyph cells exactly, which needs a stage that knows both the string and its final rect.

## Module layout

```
src/libs/ui/    antwika::ui    vocabulary, layout, draw list, painter,
                               the immediate-mode Context and its widgets
```

`add_subdirectory(ui)` goes into `src/libs/CMakeLists.txt` immediately after `gfx`; that file is dependency-ordered, not alphabetical.

`target_link_libraries(antwika_ui PUBLIC antwika::gfx)` and nothing more.
Not `antwika::log`: there is no backend, no framework and no I/O here, so there is nothing to diagnose, and `antwika_gfx` links `antwika::log` `PUBLIC` anyway, so naming it would assert a dependency that is not real.
Not `antwika::input`, `antwika::event` or `antwika::replay`, none of which this scope touches.

## The layout model

### Two node kinds, and no more

`Container` and `Text`.
A panel is a container with a background.
A button is a container with a background and a text child.
A spacer is an empty container.
That is the whole vocabulary, and it is what keeps the measure and arrange passes to one `switch` each.

### The vocabulary types

```cpp
enum class Axis : std::uint8_t { Row, Column };

enum class Alignment : std::uint8_t { Start, Center, End };

enum class SizeMode : std::uint8_t { Fixed, Fit, Grow };

struct Sizing
{
    SizeMode mode = SizeMode::Fit;
    std::uint32_t pixels = 0;

    [[nodiscard]] bool operator==(const Sizing &other) const = default;
};

inline constexpr Sizing kFit{.mode = SizeMode::Fit};
inline constexpr Sizing kGrow{.mode = SizeMode::Grow};

[[nodiscard]] constexpr Sizing fixedSize(std::uint32_t pixels) noexcept;
```

Two constants and one function rather than three factories: `kFit` and `kGrow` used in constant expressions emit no code, so there is less to cover.

### Measure -- descending index loop

A child's **demand** on an axis, as its parent sees it:

```
demand(child, axis) = child.sizing[axis].mode == SizeMode::Fixed
                        ? child.sizing[axis].pixels
                        : child.measured[axis]
```

`Fit` and `Grow` are the same in measure, deliberately: a growing child contributes its content size as a *minimum*, so "grow" means *at least my content, plus a share of anything left over*.
If a grower contributed zero, a `Fit`-height column full of growers would collapse to its padding, which makes nesting surprising.
Writing it as a two-way ternary rather than a three-way switch also avoids a third branch no test could distinguish.

```
for (i = tree.size(); i-- > 0;)
    Text:      measured = gfx::textSize(text, textScale)
    Container: mainSum  = saturating sum of demand(child, main)
               crossMax = max of demand(child, cross)
               gaps     = count > 0 ? gap * (count - 1) : 0
               measured[main]  = mainSum  + gaps + 2 * padding
               measured[cross] = crossMax        + 2 * padding
```

`count > 0 ? ... : 0` is the zero-children guard, and both directions are reachable: a panel with children, and an empty one.
Sums go through a small `saturatingAdd` helper -- reachable both ways by passing `fixedSize(0xFFFFFFFFu)` twice, so it costs no unreachable branch, and it stops a wrapped `measured` producing garbage geometry.

### Arrange -- ascending index loop

The root gets `Rect{.origin = {.x = 0, .y = 0}, .size = canvas}`, then each container arranges its own children:

**1. Content box, saturating.**
`origin + padding`, and `size > 2 * padding ? size - 2 * padding : 0` on each axis.
Both directions reachable: a normal panel, and a panel narrower than twice its padding.

**2. Early out** when there are no children.

**3. Main-axis budget.**
`mainForChildren = content.size[main] > gapTotal ? content.size[main] - gapTotal : 0`, with `gapTotal = gap * (count - 1)`.

**4. Bases.**
`base[i] = demand(child_i, main)`; `demand = sum(base)`; `growerCount` = how many children grow on the main axis.

**5. Distribution -- exactly three cases.**

```
demand < mainForChildren && growerCount > 0    grow
    slack = mainForChildren - demand
    each grower gets base + slack / growerCount
    the first (slack % growerCount) growers get one extra pixel each

demand > mainForChildren                       shrink, proportional
    final = uint32(uint64(base) * mainForChildren / demand)
    leftover from truncation goes to the earliest children, one pixel each
    demand > mainForChildren >= 0 makes demand > 0, so the division is safe
    by construction -- no guard, and so no branch a test could not reach

otherwise                                      final = base
```

**The leftover rule, stated once:** integer-division leftover always goes one pixel at a time to the earliest children in declaration order.
That is pinned by a test (`three growers in 100px -> 34, 33, 33`), not left incidental.

Shrinking is proportional rather than first-fit truncation because `IRenderer` has no scissor: proportional shrink keeps every child strictly inside its parent, so a too-small container still draws a recognisable contained picture, where first-fit would give the overflowing children zero width and lose them.

Main-axis slack with no growers is left after the last child.
There is deliberately **no main-axis alignment option**: `spacer(kGrow)` gives end-alignment, and a spacer on each side gives centring, out of layout code that already exists.
That is the whole reason `spacer` is in the widget set.

**6. Cross axis, per child.**

```
crossExtent = Fixed ? pixels : Grow ? content.size[cross] : measured[cross]
crossExtent = min(crossExtent, content.size[cross])        the containment clamp
crossOffset = Start -> 0
            | Center -> (content.size[cross] - crossExtent) / 2
            | End    ->  content.size[cross] - crossExtent
```

The clamp is what makes the `Center` and `End` subtractions provably non-underflowing, so they need no saturating guard -- and a guard that can never be false is exactly what the coverage gate would demand an impossible test for.
Worth a comment saying so.

**7. Walk.**
Place each child at `cursor` along the main axis and `crossOffset` across it, then `cursor += final + gap`.
The last child adds a trailing gap nothing reads; an `if (i + 1 < count)` would add a branch for no observable behaviour.

### Flatten

Ascending index order, which is paint order for free:

```
if (node.background) push FillRect{arranged, *background}
if (node.kind != Text) continue
if (arranged.size.height < textSize(text, scale).height) continue     no vertical clip exists
cell  = kGlyphAdvance * scale
cells = cell > 0 ? arranged.size.width / cell : 0
if (cells == 0) continue
push DrawText{arranged.origin, text.substr(0, cells), scale, textColor}
```

A background is `std::optional<Color>` rather than an `alpha == 0` sentinel: `IRenderer` states no blending contract, so overloading alpha would be a lie, and the optional gives a clean branch reachable both ways.

### Degenerate cases, all specified and all tested

| Case | Behaviour |
| --- | --- |
| `canvas == Size{0, 0}` | root content 0; every descendant gets size 0 at origin `{0, 0}`; no division by zero on either the shrink or the exact-fit path |
| Container smaller than its content | proportional shrink; everything stays inside the parent |
| More children than fit | same shrink path; every child positioned, none dropped |
| Zero children | early out; measure yields `2 * padding` on both axes |
| Padding exceeding the box | content size saturates to 0 |
| `gap * (n - 1) >= mainAvailable` | `mainForChildren == 0`; all children zero-extent |
| Deep nesting | flat loops, no recursion, no depth limit |
| Text taller than its box | text command omitted |
| Text wider than its box | truncated to whole glyph cells |
| `textScale == 0` | `gfx::textSize` returns `Size{}`; nothing emitted |

## The API

### Nesting is RAII, so mis-nesting is not expressible

`beginRow()`/`endRow()` invites imbalance, which corrupts the tree permanently.
Instead `row()`, `column()` and `panel()` return a `[[nodiscard]] Scope` whose destructor closes the container, and `Context` has **no** `end...()` method at all -- so there is nothing to check at frame end, no error to report, and no branch to cover.

```cpp
class Scope final
{
public:
    ~Scope();

    Scope(const Scope &) = delete;
    Scope(Scope &&) = delete;
    Scope &operator=(const Scope &) = delete;
    Scope &operator=(Scope &&) = delete;

private:
    friend class Context;

    explicit Scope(Context &context) noexcept;

    Context &context;
};
```

`~Scope()` does one thing: `context.closeContainer()`, which is `noexcept` because it is a single index assignment (`open = nodes[open].parent`) on an already-allocated vector.
Nothing on the close path can throw, so "destructors never throw" holds structurally rather than by care.
`closeContainer()` is private with `friend class Scope`, so a container can only be closed by the object that opened it -- the same shape as blog 012's renderer holding a `WindowId` so it has nothing to call `close()` on.

`[[nodiscard]]` matters under `-Werror`: a discarded scope would open and immediately close a container, and that becomes a compile error instead of a silent wrong picture.

`return Scope{*this};` is fine despite the deleted move constructor -- C++17 guaranteed elision initialises the return object and then the caller's variable directly, with no move.
GCC and Clang do not warn `-Wunused-variable` for a type with a non-trivial destructor, so no `[[maybe_unused]]` noise is needed.
(If that combination turns out to need adjusting during Phase 3, the fallback is a `Scope` that is move-constructible; it is a one-line change and touches nothing else.)

### `Context`

```cpp
class Context final
{
public:
    Context(Size canvas, Theme theme);
    ~Context();

    Context(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

    [[nodiscard]] const Theme &theme() const noexcept;

    [[nodiscard]] Scope row(ContainerSpec spec = {});
    [[nodiscard]] Scope column(ContainerSpec spec = {});
    [[nodiscard]] Scope panel(ContainerSpec spec = {});

    void label(std::string_view text);
    void label(std::string_view text, Color color);

    void button(std::string_view text, ButtonState state = ButtonState::Idle);
    void button(
        std::string_view text, Sizing width, ButtonState state);

    void spacer(Sizing along);

    [[nodiscard]] DrawList finish();
};
```

`Context` holds no reference to anything -- there is no retained state in this scope -- but it declares all four copy/move operations `= delete` anyway, since it owns a frame arena and relocating one mid-build is never wanted.

`finish()` runs `layout()` then `flatten()` and returns the commands.
`Context` holds `std::unique_ptr<Frame>` and therefore declares `~Context();` defined in the `.cpp`, the pimpl idiom the style guide already names for `World`, because `Frame` and `LayoutTree` are private headers a public header cannot include.
Cost is one allocation per frame, negligible beside the per-widget string copies.

`ContainerSpec` carries the per-call options, so nothing about sizing lives in the theme:

```cpp
struct ContainerSpec
{
    Sizing width = kGrow;
    Sizing height = kFit;
    Alignment cross = Alignment::Start;
    std::optional<Color> background{};
    std::optional<std::uint32_t> padding{};
    std::optional<std::uint32_t> gap{};
};
```

`panel` differs from `column` only in what an unset field resolves to -- `background` falls back to `theme.panel` rather than none, and `padding` to `theme.padding` rather than 0.
One private `resolve()` helper covers it.

`spacer(along)` reads the open container's axis and applies `along` to the main axis and `kFit` across it, so `spacer(kGrow)` means "push the rest away" in either orientation.

There is no separate `box` widget: a childless `panel` with fixed sizing *is* a coloured rectangle, which is what poker's stack bar needs.

### Buttons, without input

A button is a container with the theme's button background, `theme.buttonPadding`, `cross = Alignment::Center` for vertical centring, and internally `spacer(kGrow)`, the label, `spacer(kGrow)` for horizontal centring -- so it is built from layout machinery that already exists rather than from a special case.

```cpp
enum class ButtonState : std::uint8_t { Idle, Hovered, Pressed };
```

`ButtonState` picks which of three theme colours the background uses, **and the caller supplies it**.
Nothing in this library computes it: there is no pointer here, so there is nothing to compute it from.
An app can still drive it from its own state -- highlighting whose turn it is, say -- and it is the seam interaction would later plug into without changing a signature.
Default `Idle`, so `ui.button("fold")` is the common case.

With an odd amount of slack the two centring spacers split it with the extra pixel going to the first, so the label sits one pixel left of true centre.
Deterministic, and cheaper than a main-axis alignment mode.

### `Theme`

A plain value with every field defaulted, so `Theme{}` is the default theme and overriding one colour is one designated initialiser:

```cpp
struct Theme
{
    Color panel{...};
    Color text{...};
    Color muted{...};
    Color buttonIdle{...};
    Color buttonHovered{...};
    Color buttonPressed{...};
    Color buttonText{...};

    std::uint32_t textScale = 1;
    std::uint32_t padding = 4;
    std::uint32_t gap = 4;
    std::uint32_t buttonPadding = 6;
};
```

In it: colours and metrics a widget picks without being told.
Not in it: sizing and alignment, which are per call, and anything retained.

Poker's private `scaleFor(canvas)` heuristic is real prior art and gets promoted to two free functions beside the theme:

```cpp
[[nodiscard]] std::uint32_t scaleForCanvas(Size canvas) noexcept;
[[nodiscard]] Theme scaledTheme(Theme base, std::uint32_t scale) noexcept;
```

so an app writes `ui::Context ui{canvas, ui::scaledTheme(ui::Theme{}, ui::scaleForCanvas(canvas))}` and an app wanting a fixed scale simply does not call them.

### No error type

There is no failure category here.
Bad sizes saturate, oversized text truncates, an empty container lays out to its padding, and mis-nesting is not expressible.
So no `UiError` is introduced -- an exception type with no throw site would be both dead code and uncoverable.

### Usage

```cpp
void DemoUi::describe(ui::Context &ui) const
{
    const auto screen = ui.panel({.height = ui::kGrow});

    ui.label("Antwika UI");

    {
        const auto body = ui.row({.height = ui::kGrow});

        {
            const auto side =
                ui.panel({.width = ui::fixedSize(120), .height = ui::kGrow});

            ui.label("layouts");
            ui.label("buttons");
            ui.label("text");
        }

        {
            const auto main = ui.column({.height = ui::kGrow});

            ui.label("nested rows and columns", ui.theme().muted);
            ui.spacer(ui::kGrow);

            {
                const auto actions = ui.row({.cross = ui::Alignment::Center});

                ui.spacer(ui::kGrow);
                ui.button("ok");
                ui.button("cancel");
            }
        }
    }
}
```

## Files

### `src/libs/ui/include/antwika/ui/` -- public, installed

| File | Purpose |
| --- | --- |
| `Axis.hpp` | which way a container stacks its children |
| `Alignment.hpp` | cross-axis placement of a child |
| `Sizing.hpp` | `SizeMode`, `Sizing`, `kFit`, `kGrow`, `fixedSize()` |
| `ContainerSpec.hpp` | per-call container options |
| `ButtonState.hpp` | the three appearances a button can be asked for |
| `Theme.hpp` | colours and metrics, `scaleForCanvas()`, `scaledTheme()` |
| `DrawCommand.hpp` | `FillRect`, `DrawText`, and the variant over them |
| `DrawList.hpp` | `using DrawList = std::vector<DrawCommand>;` |
| `Scope.hpp` | the `[[nodiscard]]` RAII container guard |
| `Context.hpp` | the immediate-mode façade, the only type app code drives |
| `Painter.hpp` | `void paint(IRenderer &renderer, const DrawList &commands);` |

### `src/libs/ui/src/` -- private headers and implementation

| File | Purpose |
| --- | --- |
| `NodeKind.hpp` | `Container` and `Text`, the only two |
| `Node.hpp` | one arena entry: kind, sizing, style, tree links, measured, arranged |
| `LayoutTree.hpp` / `.cpp` | the flat arena and its open/append/close API |
| `Layout.hpp` / `.cpp` | `void layout(LayoutTree &tree, Size canvas);` -- the two flat passes |
| `Flatten.hpp` / `.cpp` | `DrawList flatten(const LayoutTree &tree);` |
| `Frame.hpp` | `Context`'s pimpl, holding the `LayoutTree` |
| `Theme.cpp`, `Scope.cpp`, `Context.cpp`, `Painter.cpp` | implementations |

Keeping `Node`, `LayoutTree`, `layout()` and `flatten()` private keeps the public surface to eleven small types instead of exposing a fifteen-field `Node`.
Tests reach them the way `src/libs/gfx/tests/NullRendererTest.cpp` already reaches a private class: `target_include_directories(antwika_ui_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)`.

### `src/libs/ui/tests/`

`LayoutTreeTest.cpp`, `LayoutTest.cpp`, `LayoutDegenerateTest.cpp`, `FlattenTest.cpp`, `SizingTest.cpp`, `ThemeTest.cpp`, `DrawCommandTest.cpp`, `ScopeTest.cpp`, `ContextTest.cpp`, `ContextNestingTest.cpp`, `ContextWidgetsTest.cpp`, `PainterTest.cpp`.
Splitting `Layout` and `Context` across a few files follows the existing `Solver*Test.cpp` / `Table*Test.cpp` precedent.

**No `mocks/` or `fakes/` directory.**
`PainterTest.cpp` links the existing `antwika::gfx::tests::mocks` and uses `MockRenderer`; everything else is a value comparison.
That sidesteps `scripts/check_unused_test_doubles.py` entirely, and it is the strongest evidence the design is right -- a library whose behaviour is fully observable as values needs no doubles of its own.
No new interfaces either: `IRenderer` is already the one seam worth mocking, and nothing else here has a collaborator to isolate.

## Phases

Each phase ends with a green build and a full `ctest` run, and is done in its own git worktree per `CLAUDE.md`.

### Phase 1 -- the vocabulary and the layout core

`src/libs/ui/` with `Axis`, `Alignment`, `Sizing`, `NodeKind`, `Node`, `LayoutTree` and `layout()`.
`add_subdirectory(ui)` after `gfx` in `src/libs/CMakeLists.txt`, and `antwika_ui_tests` into the non-MinGW expected-binary list in `.github/workflows/build.yml` (sorted, between `antwika_time_tests` and `antwika_wfc_tests`; the MinGW list is apps only and needs no change).

Tests build a `LayoutTree` by hand and assert `arranged` rects as exact pixel values.
This is where the algorithm is pinned, and it is the largest surface in the library.
Nothing else in the repository changes, and nothing can draw yet.

### Phase 2 -- the draw list and the painter

`DrawCommand`, `DrawList`, `flatten()` and `paint()`.
The library can now turn a hand-built tree into `IRenderer` calls.
`PainterTest.cpp` is the one file in the library that touches `MockRenderer`.

Keeping this apart from Phase 1 means the layout algorithm is provable before anything can draw it.

### Phase 3 -- the immediate-mode façade

`Theme`, `ContainerSpec`, `ButtonState`, `Frame`, `Scope` and `Context`, with `row`, `column`, `panel`, `label`, `button`, `spacer` and `finish()`.
Plus `scaleForCanvas()` and `scaledTheme()`.

This is the phase that makes the library usable, and the phase where the RAII scope pattern is confirmed under `-Werror` on all three toolchains.

### Phase 4 -- the UI in `gfx_demo`

`DemoScene` becomes a `describe(ui::Context &)` pass plus `paint()`, showing a nested layout: a panel containing a row containing a fixed-width sidebar column and a growing main column, with labels and a right-aligned button row.

`gfx_demo` is the right first home: it runs its own frame loop with no engine and no replay, so there is nothing to corrupt, and `DemoLoopTest`/`DemoSceneTest` already run against `MockGfxBackend`/`MockRenderer`.
Its test asserts the resulting `DrawList` as a value, which is a strictly better assertion than the current exact-rect `InSequence` pinning.

This is the first thing a person can look at: under `sdl3` or `raylib` it is a real window with real nested layouts, buttons and text; under `null` it draws nothing, as ever.

### Phase 5 -- documentation

`REQUIREMENTS.md`, `CLAUDE.md`, `README.md` and `docs/ui-plan.md` (this document), as listed below.
A `blog/013-*.md` write-up is optional and, per `CLAUDE.md`, written after the fact rather than as part of the work.

## Testing and coverage

Everything under `src/libs/ui/` is inside the gcovr filter and held to 100% line, function and branch coverage on the GNU leg.
The design is built for that: pure functions over values, one mock in one file.

Branch checklist, written down now so coverage is not chased later:

1. Content-box padding saturation, both directions.
2. Zero children and some children.
3. `gapTotal >= mainAvailable` and `<`.
4. Distribution: slack with growers, slack without growers, exact fit, shrink.
5. Leftover remainder zero and non-zero, on both the grow and the shrink path.
6. Cross-axis containment clamp, both directions.
7. `Alignment::Start`, `Center`, `End`.
8. Each `SizeMode` on each axis, on both `Axis::Row` and `Axis::Column`.
9. `saturatingAdd` clamping and not clamping.
10. Flatten: background present and absent; text fitting, truncated horizontally, dropped vertically; `textScale == 0`.
11. `LayoutTree`: first-child append and subsequent-child append.
12. `panel` versus `row` spec resolution, each optional set and unset.
13. `spacer` inside a row and inside a column.
14. `scaleForCanvas` below and above its threshold.

Two `// GCOVR_EXCL_LINE` shapes are expected, both already documented in `docs/confirming-unreachable-branches.md`:
the closing braces of `flatten()`, `Context::finish()` and any function holding a local `std::string`/`std::vector` (case b, unwind landing pads), and `LayoutTree`'s `nodes.push_back(Node{...})` throw branch (case a, same shape as `SystemScheduler::createPhase`).

Note that public headers count towards the gate, so every inline or `constexpr` function in `include/antwika/ui/` needs a runtime call from a test.
That is the reason `operator==` is defined only on `Sizing`, `FillRect`, `DrawText` and `DrawCommand` -- the types tests actually compare -- and not on `Theme` or `ContainerSpec`, where a defaulted comparison would be a chain of short-circuit branches CI would demand coverage for.

## Documentation and requirements to update

`REQUIREMENTS.md` gains Must-haves:

- UI layout must be a pure function of the described UI and the canvas size, computed arithmetically from the built-in font's metrics without asking a graphics backend to measure anything.
- Layouts must nest, and a container's size must be derivable from its children's content.
- A widget must never draw outside its container, since the graphics abstraction offers no clipping.
- No file under `src/libs/ui/` may consult a clock or any source of state outside its arguments, so the same described UI and canvas size always produce the same drawing commands.
- Integer-division leftover in a layout must be distributed by a specified, tested rule rather than incidentally.

`REQUIREMENTS.md` gains a Should-have:

- A UI's appearance should be expressible as a value (a list of drawing commands), so a picture can be asserted as data rather than looked at.

`REQUIREMENTS.md` gains Won't-haves, one line each from [Deferred deliberately](#deferred-deliberately).

`CLAUDE.md` gains an `antwika::ui` paragraph in Architecture, after the `antwika::gfx` one where the write-only-projection story already lives.
`README.md` gains a library-list entry and a mention in the `gfx_demo` description.

Nothing else needs touching: no new dependency, so `conanfile.py` and the lockfiles are untouched; `scripts/check_line_length.py`, `scripts/check_one_sentence_per_line.py` and `scripts/check_unused_test_doubles.py` already glob `src/**`; the `gfx-backends` job needs no change, since there is no UI conformance suite (see below).

Two things are deliberately *not* touched.
`REQUIREMENTS.md:51`, `:68` and `:74` still defer live input capture; this scope needs none of it, and editing them here would claim a capability the tree does not have.
And `docs/input-plan.md` is left alone -- though note in passing that its `:352-358` recommendation (persist a raw click, derive "toggle the cell at (3, 4)" downstream from canvas-dependent grid geometry) is a latent determinism bug, since the derivation depends on a window size that is never recorded. That belongs to whoever picks the input work up, and it is worth raising then.

## Deferred deliberately

- **All pointer and keyboard input, hit-testing and interaction.**
  This is the scope decision, not an oversight.
  *Pointer input and clickable buttons have since been built -- see [`docs/ui-input-plan.md`](ui-input-plan.md).
  Keyboard input, focus and any widget carrying a value are still deferred, for the reasons that document gives.*
  `antwika::input` is vocabulary-only today -- no folded device state, no input backend under `backends/`, and `NullInputBackend::pollEvent()` always returns nullopt -- so no button could be clicked in any build that exists.
  Adding interaction later is additive: the described tree is already the thing a hit-test would consult, and `ButtonState` is already the seam a resolved hover would feed.
- **Anything crossing back into the engine or a replay.**
  Rendering stays a write-only projection.
  When interaction does arrive, the thing that may cross is a *symbolic* widget identity translated into an event the app already owns, never a pixel -- hit-testing depends on the canvas size, so a recorded click would resolve to a different widget at a different window size and silently break replay determinism.
  That argument is worth writing down when it is needed; it does not need building now.
- **Clipping and therefore scrolling.**
  `IRenderer` has no scissor, and adding one means a clip stack in every directory under `backends/` plus a way for `GfxBackendConformance` to prove it -- a `gfx` change, not a `ui` change.
  Scrolling without clipping draws outside its container, so the two defer together.
  Until then, containment is the layout's job, which is why overflow shrinks proportionally.
- **Z-order, overlapping panels, popups and modals.**
  These need a draw order independent of tree order *and* a hit-test priority independent of draw order, two orderings to keep consistent, and a popup extending past its parent needs clipping anyway.
- **Multi-line text and wrapping.**
  `gfx::textSize()` measures one line.
  Wrapping is a layout policy -- break on what, hyphenate, ellipsise -- that only earns its complexity against real prose, and none exists here.
- **Main-axis alignment as an option.**
  `spacer(kGrow)` already expresses start, end and centre out of code that exists, and a `MainAlign` enum would add three branches to cover for no new capability.
- **Weighted grow.**
  Growers split slack equally, leftover to the earliest.
  A `2:1` split needs a weight on `Sizing` and a second distribution rule, and nothing wants it yet.
- **Animation and anything time-dependent.**
  `antwika::ui` reads no clock, deliberately, so the same state and canvas always give the same picture.
- **Theming beyond a plain struct.**
  One `Theme` value, no style stack, no cascade, no per-widget override chain.
  A cascade is retained state during a build pass, and it buys nothing until there is a second visual style.
- **Sliders, checkboxes, text fields and anything carrying a value.**
  All of them are interaction, and all of them widen the picture-only contract.
- **Migrating `apps/poker`'s `TableScene` onto `antwika::ui`.**
  A genuine deduplication win -- it would retire the private `scaleFor`, `lineHeight` and three hand-rolled centring sites -- but it is a refactor of working, fully-tested code with a hand-history layout to preserve, and it should follow the library rather than ship with it.
- **A UI conformance suite.**
  The conformance idiom exists for code living outside the coverage gate under `backends/`.
  All of `antwika::ui` sits inside the gate and is held to 100% like every other library, so a conformance suite would be a second, weaker copy of the unit tests.
- **Multi-window UI.**
  One canvas per `Context`.
  Nothing here prevents two, and nothing needs two.

## What changed while building it

Five things came out differently from the plan above.

**No `Frame` struct.**
`Context` forward-declares `detail::LayoutTree` and holds a `std::unique_ptr` to it directly, which is the same pimpl idiom with one fewer type in it.

**`Node` and `LayoutTree` got a `Saturate.hpp`.**
The saturating narrow-to-32-bit helper is shared by the layout and by `scaledTheme()`, so it lives in one private header rather than being copied into two translation units.

**No `switch` on `Alignment`, and none on `ButtonState`.**
GCC emits a fourth, out-of-range arm for a `switch` over a three-value `std::uint8_t` enum, reachable only by constructing an invalid enumerator.
Nothing can, so no test could ever cover it, and the coverage gate would have demanded either a dishonest test or an exclusion that also dropped the three real arms from the denominator.
Both are written as `if`/`else if` chains with the first case as the fall-through.
The cost is losing `-Wswitch` exhaustiveness if a fourth enumerator is ever added, and that trade is noted at both sites.

**`button()` takes its width as a third argument** rather than through a second overload, and there is no separate `box` widget: a childless `panel` with a fixed size already is one.

**Six `GCOVR_EXCL_LINE` markers, not two.**
The plan predicted the two exception-unwind shapes in `flatten()`; `Context` has the same shape at every point it builds a `Node`, because a `Node` carries a `std::string` and so has an unwind path that frees it.
One comment at the top of `Context.cpp` explains the whole class of them.
The alternative is worth recording: if `Node` held an index into a string table owned by `LayoutTree` instead of a `std::string`, it would become a trivially destructible POD and all six would disappear, along with the allocation per node.
That is a genuine improvement and a reasonable follow-up, but it is a refactor of the arena rather than part of this work.

## Verification

Per phase, from the repo root inside a dev container, in the phase's own worktree:

```sh
conan install . -of build \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan.lock
cmake --preset conan-release
cmake --build build -j24
build/bin/antwika_ui_tests                 # fast inner loop, phases 1-3
ctest --test-dir build --output-on-failure  # before calling a phase done
```

Coverage, which is the gate that actually decides whether a phase is finished:

```sh
cmake --preset conan-coverage
cmake --build build-coverage -j24
ctest --test-dir build-coverage
gcovr --root . --filter 'src/libs/ui/.*' --exclude '.*/tests/.*' --print-summary build-coverage
```

Then the same `--filter 'src/.*'` run CI does, plus `python3 scripts/check_full_coverage.py --summary coverage-summary.json`, to confirm the whole tree is still at 100%.

Style gates, all three of which CI runs:

```sh
python3 scripts/check_line_length.py
python3 scripts/check_one_sentence_per_line.py
python3 scripts/check_unused_test_doubles.py
```

End of Phase 4, the part that needs eyes rather than a test -- a real window with a real nested layout:

```sh
scripts/update_lockfiles.sh          # only if a lockfile is stale
conan install . -of build-sdl3 -o gfx_backend=sdl3 \
  -c tools.cmake.cmake_layout:build_folder_vars="['options.gfx_backend']" \
  -pr:b=./profiles/build/${CONAN_PROFILE} \
  -pr:h=./profiles/host/${CONAN_PROFILE} \
  --build=missing -s build_type=Release --lockfile=conan-sdl3.lock
cmake --preset conan-gfx_backend_sdl3-release
cmake --build build-sdl3 -j24
xvfb-run -a build-sdl3/bin/antwika_gfx_demo     # or with a display, run it directly
ctest --test-dir build-sdl3 --output-on-failure
```

And the check that the abstraction did not quietly become one backend's shape -- the same demo under `raylib`, and under the default `null` backend where it must draw nothing and still exit cleanly.
