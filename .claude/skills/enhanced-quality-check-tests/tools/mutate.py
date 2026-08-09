"""Break the production code on purpose and report what nothing noticed.

Reading produces an argument; mutation produces an answer. A mutant that
survives is a fault no test in the suite detects, which is the sharpest
statement available about a gap in the tests.

    python3 mutate.py --module libs/wfc
    python3 mutate.py --module apps/game --limit 40 --seed 7
    python3 mutate.py --module libs/ui --cores 0-1

A rebuild per mutant is the cost of the run, so what it may take is the
knob that matters when several of these run at once: eight unbounded runs
on a 24-core machine ask for 192 compilers and take the machine down.

So a run is always pinned, for every module, whether it asked to be or
not. Three ways the CPU set is settled, first one wins:

    --cores 0-3             this run gets those four
    MUTATE_CORES=0-3        every run in this shell does, apps included
    neither                 all but RESERVED_CORES of the affinity mask

Pinning happens before anything forks, so make, every compiler and the
test binary inherit it and the kernel is what enforces the limit. --jobs
sets `cmake --build -j` alone, which bounds the build if make cooperates
and bounds nothing else; it defaults to the pinned core count, so the two
cannot drift apart by accident. The header line prints what was settled
on and what settled it, before the run spends any of it.

Each mutant is applied to one source file, the module's test target is
rebuilt, its binary is run, and the file is restored. The tree is left
exactly as it was found, including when interrupted: SIGINT, SIGTERM and
SIGHUP all restore, and so does an exception that reaches no handler.
Only SIGKILL can leave a mutant behind.

Which tree that is comes from the working directory, not from __file__.
A worktree checks out its own copy of this script, but an agent standing
in a worktree can still invoke the project root's copy by absolute path,
and the project root must not be what gets mutated. --repo overrides.

A surviving mutant is not automatically a missing test: some are
equivalent (they change nothing observable) and some sit behind a
GCOVR_EXCL marker. Read every survivor before acting on it.
"""
import argparse
import atexit
import os
import pathlib
import random
import re
import signal
import subprocess
import sys

# The tree to mutate, settled in main() from the working directory rather
# than from __file__. A worktree checks out its own copy of this script,
# but an agent standing in a worktree may still invoke the project root's
# copy by absolute path, and that must not mutate the project root.
REPO = pathlib.Path(__file__).resolve().parents[4]

SCRIPT_TREE = REPO

# Cores held back from a run that named none, so the machine stays usable.
RESERVED_CORES = 2

# Names the limit once for a whole fan-out, so every module inherits it.
CORES_ENV = "MUTATE_CORES"

OPERATORS = [
    (re.compile(r"(?<= )<=(?= )"), "<", "relational <= to <"),
    (re.compile(r"(?<= )>=(?= )"), ">", "relational >= to >"),
    (re.compile(r"(?<= )<(?= )"), "<=", "relational < to <="),
    (re.compile(r"(?<= )>(?= )"), ">=", "relational > to >="),
    (re.compile(r"(?<= )==(?= )"), "!=", "equality == to !="),
    (re.compile(r"(?<= )!=(?= )"), "==", "equality != to =="),
    (re.compile(r"(?<= )&&(?= )"), "||", "boolean && to ||"),
    (re.compile(r"(?<= )\|\|(?= )"), "&&", "boolean || to &&"),
    (re.compile(r"(?<= )\+(?= )"), "-", "arithmetic + to -"),
    (re.compile(r"(?<= )-(?= )"), "+", "arithmetic - to +"),
    (re.compile(r"(?<= )\*(?= )"), "/", "arithmetic * to /"),
    (re.compile(r"\btrue\b"), "false", "literal true to false"),
    (re.compile(r"\bfalse\b"), "true", "literal false to true"),
]

LITERAL = re.compile(r"(?<![\w.'\"])(\d+)(?![\w.'\"])")

SKIP_LINE = re.compile(
    r"GCOVR_EXCL|#include|#pragma|static_assert|^\s*//|^\s*\*")


def module_paths(module):
    """Sources to mutate, the CMake target, and the test binary."""
    source_root = REPO / "src" / module
    if not source_root.is_dir():
        raise SystemExit(f"no such module: src/{module}")

    name = "antwika_" + module.split("/", 1)[1].replace("/", "_")
    target = f"{name}_tests"
    binary = REPO / "build" / "bin" / name / target

    sources = []
    for pattern in ("src/*.cpp", "src/**/*.cpp", "include/**/*.hpp",
                    "src/*.hpp"):
        for path in sorted(source_root.glob(pattern)):
            if "/tests/" not in str(path):
                sources.append(path)
    return sorted(set(sources)), target, binary


def candidates(sources):
    """Every (path, line index, column, replacement, label) we could apply."""
    out = []
    for path in sources:
        lines = path.read_text(errors="replace").splitlines(keepends=True)
        for index, line in enumerate(lines):
            if SKIP_LINE.search(line):
                continue
            for pattern, replacement, label in OPERATORS:
                for m in pattern.finditer(line):
                    out.append(
                        (path, index, m.start(), m.end(), replacement, label))
            for m in LITERAL.finditer(line):
                bumped = str(int(m.group(1)) + 1)
                out.append(
                    (path, index, m.start(), m.end(), bumped,
                     f"literal {m.group(1)} to {bumped}"))
    return out


def apply_mutation(mutation):
    """Write one mutant, returning the original file text."""
    path, index, start, end, replacement, _ = mutation
    original = path.read_text(errors="replace")
    lines = original.splitlines(keepends=True)
    line = lines[index]
    lines[index] = line[:start] + replacement + line[end:]
    path.write_text("".join(lines))
    return original


def available_cores():
    """The cores this process may use, which taskset narrows and nproc does not."""
    try:
        return set(os.sched_getaffinity(0))
    except AttributeError:                      # not Linux
        return set(range(os.cpu_count() or 1))


def positive(text):
    """An argparse type for counts that make no sense at zero."""
    value = int(text)
    if value < 1:
        raise argparse.ArgumentTypeError(f"must be at least 1, got {value}")
    return value


def core_list(text):
    """A taskset-style CPU list: 0-3, or 0,2,4, or 6."""
    def number(field, part):
        try:
            return int(field)
        except ValueError:
            raise argparse.ArgumentTypeError(
                f"not a core number: {part!r}") from None

    cores = set()
    for part in text.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part.lstrip("-"):
            low, _, high = part.partition("-")
            first, last = number(low, part), number(high, part)
            if last < first:
                raise argparse.ArgumentTypeError(f"empty range: {part}")
            cores.update(range(first, last + 1))
        else:
            cores.add(number(part, part))
    if not cores:
        raise argparse.ArgumentTypeError(f"names no cores: {text!r}")
    if min(cores) < 0:
        raise argparse.ArgumentTypeError(f"negative core: {text!r}")
    return cores


def format_cores(cores):
    """Render a CPU set the way it was asked for: 0-3 rather than 0,1,2,3."""
    spans = []
    for core in sorted(cores):
        if spans and core == spans[-1][1] + 1:
            spans[-1][1] = core
        else:
            spans.append([core, core])
    return ",".join(str(first) if first == last else f"{first}-{last}"
                    for first, last in spans)


def default_cores(available):
    """A limit for the run that said nothing, so there is always one."""
    keep = max(1, len(available) - RESERVED_CORES)
    return set(sorted(available)[:keep])


def settle_cores(requested):
    """Which CPUs this run may use, and what decided that. Never everything."""
    available = available_cores()

    if requested is not None:
        return requested, "--cores"

    named = os.environ.get(CORES_ENV)
    if named:
        try:
            return core_list(named), CORES_ENV
        except argparse.ArgumentTypeError as bad:
            raise SystemExit(f"{CORES_ENV}={named!r}: {bad}")

    return default_cores(available), f"default, reserving {RESERVED_CORES}"


def restrict_to(cores):
    """Pin this process, so every child the run spawns inherits the limit."""
    if not hasattr(os, "sched_setaffinity"):
        raise SystemExit("a core limit needs sched_setaffinity, which this "
                         "platform does not have; use --jobs instead")

    allowed = available_cores()
    missing = sorted(cores - allowed)
    if missing:
        raise SystemExit(f"cores not available to this process: {missing}; "
                         f"available: {format_cores(allowed)}")

    os.sched_setaffinity(0, cores)


def repo_root(explicit):
    """The tree to mutate: the one you are standing in, not the one this
    script lives in. An agent in a worktree that invokes the project root's
    copy of this file must mutate its worktree, not the project root."""
    if explicit is not None:
        root = pathlib.Path(explicit).resolve()
        if not (root / ".git").exists():
            raise SystemExit(f"--repo is not a git worktree: {root}")
        return root

    try:
        done = subprocess.run(["git", "rev-parse", "--show-toplevel"],
                              capture_output=True, text=True, check=True)
    except (OSError, subprocess.CalledProcessError):
        raise SystemExit("not inside a git worktree and no --repo given; "
                         "run this from the tree you mean to mutate") from None

    return pathlib.Path(done.stdout.strip()).resolve()


def run(command, cwd=None, timeout=1800):
    try:
        done = subprocess.run(
            command, cwd=cwd or REPO, capture_output=True, text=True,
            timeout=timeout)
        return done.returncode
    except subprocess.TimeoutExpired:
        return 124


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--module", required=True,
                        help="e.g. libs/wfc or apps/game")
    parser.add_argument("--limit", type=positive, default=25)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--repo", default=None,
                        help="tree to mutate; defaults to the git worktree "
                             "of the working directory, never to wherever "
                             "this script happens to live")
    parser.add_argument("--cores", type=core_list, default=None,
                        help="CPUs this run may use, taskset-style (0-3, "
                             "0,2,4); pins the process so make, the "
                             "compilers and the test binary inherit it. "
                             f"${CORES_ENV} sets it for a whole fan-out")
    parser.add_argument("--jobs", "-j", type=positive, default=None,
                        help="compilers per rebuild; defaults to the number "
                             "of cores the run is pinned to")
    options = parser.parse_args()

    # Pin before anything forks, and whatever the module: the limit is not
    # a flag the caller may forget, and libs and apps cost the same.
    cores, decided_by = settle_cores(options.cores)
    restrict_to(cores)

    if options.jobs is None:
        options.jobs = len(cores)

    global REPO
    REPO = repo_root(options.repo)

    sources, target, binary = module_paths(options.module)
    if not sources:
        raise SystemExit(f"no sources found under src/{options.module}")

    every = candidates(sources)
    if not every:
        raise SystemExit("no mutable sites found")

    # Fail before the first mutation rather than after it: an unbuilt tree
    # would otherwise break every source in turn and report nothing useful.
    if not binary.exists():
        raise SystemExit(f"test binary not built: {binary}\n"
                         f"build it first: cmake --build "
                         f"{options.build_dir} --target {target}")

    random.Random(options.seed).shuffle(every)
    chosen = every[:options.limit]

    print(f"repo        {REPO}")
    if SCRIPT_TREE != REPO:
        print(f"            (script lives in {SCRIPT_TREE}; that tree is "
              f"not touched)")
    print(f"module      src/{options.module}")
    print(f"sources     {len(sources)}")
    print(f"sites       {len(every)} ({len(chosen)} sampled)")
    print(f"target      {target}")
    print(f"cores       {len(cores)} of {os.cpu_count()} "
          f"({format_cores(cores)}) [{decided_by}]")
    print(f"jobs        {options.jobs}")
    print()

    restore = {}

    def cleanup(*_):
        for path, text in restore.items():
            path.write_text(text)
        restore.clear()

    def bail(number, _frame):
        cleanup()
        sys.exit(128 + number)

    # SIGINT alone is not enough: pkill and most supervisors send SIGTERM,
    # and a run killed mid-mutant leaves the source broken behind it.
    for name in ("SIGINT", "SIGTERM", "SIGHUP"):
        if hasattr(signal, name):
            signal.signal(getattr(signal, name), bail)

    # Covers the exception that reaches no handler at all.
    atexit.register(cleanup)

    survived = []
    killed = 0
    broken = 0

    for number, mutation in enumerate(chosen, start=1):
        path, index, start, _, replacement, label = mutation
        where = f"{path.relative_to(REPO)}:{index + 1}"

        original = apply_mutation(mutation)
        restore[path] = original
        try:
            build = run(["cmake", "--build", options.build_dir,
                         "--target", target, "-j", str(options.jobs)])
            if build != 0:
                broken += 1
                verdict = "did not compile"
            else:
                code = run([str(binary), "--gtest_brief=1"])
                if code == 0:
                    survived.append((where, label))
                    verdict = "SURVIVED"
                else:
                    killed += 1
                    verdict = "killed"
        finally:
            path.write_text(original)
            restore.pop(path, None)

        print(f"[{number:3d}/{len(chosen)}] {verdict:16s} {label:24s} {where}")

    # Leave the tree building again.
    run(["cmake", "--build", options.build_dir, "--target", target,
         "-j", str(options.jobs)])

    print()
    print(f"killed      {killed}")
    print(f"survived    {len(survived)}")
    print(f"uncompilable {broken}")
    total = killed + len(survived)
    if total:
        print(f"score       {100 * killed // total}%")
    if survived:
        print()
        print("survivors — a fault no test detects:")
        for where, label in survived:
            print(f"    {where}  {label}")
    return 1 if survived else 0


if __name__ == "__main__":
    sys.exit(main())
