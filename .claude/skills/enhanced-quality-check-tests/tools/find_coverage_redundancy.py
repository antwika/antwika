"""Attribute coverage to each test, then report which tests add nothing.

This is layer A2 of the redundancy question and the one that makes a
deletion safe. It runs each test in a module on its own against an
instrumented build and records the lines it covers.

    python3 find_coverage_redundancy.py --module libs/wfc

Three answers come out of one pass:

  load-bearing — the only test covering some line. Never delete it; the
  100% gate depends on it.

  subsumed — every line it covers is covered by one other test as well.
  A candidate for deletion, to be confirmed by mutation, never on this
  evidence alone.

  covers nothing — the test executes no production line in this module,
  which usually means it is testing the test fixture.

Coverage subsumption is a *necessary* condition for redundancy, not a
sufficient one. Two tests can cover identical lines and assert entirely
different things about them. Confirm with mutate.py before deleting.
"""
import argparse
import collections
import json
import pathlib
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[4]


def module_paths(module, build_dir):
    name = "antwika_" + module.split("/", 1)[1].replace("/", "_")
    target = f"{name}_tests"
    binary = REPO / build_dir / "bin" / name / target
    notes = sorted(
        (REPO / build_dir / "src" / module / "CMakeFiles").rglob("*.gcno"))
    return target, binary, notes


def run(command, timeout=900):
    return subprocess.run(
        command, cwd=REPO, capture_output=True, text=True, timeout=timeout)


def list_tests(binary):
    out = run([str(binary), "--gtest_list_tests"]).stdout
    suite = ""
    names = []
    for line in out.splitlines():
        if not line.strip() or line.startswith("Running"):
            continue
        if not line.startswith(" "):
            suite = line.strip().rstrip(".")
        else:
            name = line.strip().split("#")[0].strip()
            if suite and name:
                names.append(f"{suite}.{name}")
    return names


def covered_lines(notes):
    """The (file, line) pairs the current profiles say ran.

    `gcov --json-format --stdout` costs milliseconds where a full gcovr
    pass costs seconds, which is what makes a per-test sweep practical.
    """
    if not notes:
        return set()

    done = run(["gcov", "--json-format", "--stdout"]
               + [str(n) for n in notes])
    hit = set()
    for chunk in done.stdout.splitlines():
        chunk = chunk.strip()
        if not chunk.startswith("{"):
            continue
        try:
            payload = json.loads(chunk)
        except json.JSONDecodeError:
            continue
        for entry in payload.get("files", []):
            name = entry.get("file", "")
            if "/tests/" in name or name.startswith("/usr"):
                continue
            for line in entry.get("lines", []):
                if line.get("count", 0) > 0:
                    hit.add((name, line["line_number"]))
    return hit


def clear_profiles(module, build_dir):
    for path in (REPO / build_dir).rglob("*.gcda"):
        if module.split("/")[-1] in str(path):
            path.unlink()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--module", required=True)
    parser.add_argument("--build-dir", default="build-coverage")
    parser.add_argument("--limit", type=int, default=0,
                        help="stop after N tests (0 means all)")
    options = parser.parse_args()

    target, binary, notes = module_paths(options.module, options.build_dir)
    if not binary.exists():
        raise SystemExit(
            f"no instrumented binary at {binary}\n"
            f"build it first: cmake --build {options.build_dir} "
            f"--target {target}")

    names = list_tests(binary)
    if options.limit:
        names = names[:options.limit]
    print(f"module   src/{options.module}")
    print(f"tests    {len(names)}")
    print()

    per_test = {}
    for number, name in enumerate(names, start=1):
        clear_profiles(options.module, options.build_dir)
        run([str(binary), f"--gtest_filter={name}", "--gtest_brief=1"])
        per_test[name] = covered_lines(notes)
        print(f"[{number:4d}/{len(names)}] {len(per_test[name]):5d} lines  "
              f"{name}")

    owners = collections.Counter()
    for lines in per_test.values():
        owners.update(lines)

    load_bearing = []
    subsumed = []
    empty = []

    for name, lines in per_test.items():
        if not lines:
            empty.append(name)
            continue
        if any(owners[line] == 1 for line in lines):
            load_bearing.append(name)
            continue
        for other, others in per_test.items():
            if other != name and lines <= others:
                subsumed.append((name, other))
                break

    print()
    print(f"load-bearing   {len(load_bearing)}")
    print(f"subsumed       {len(subsumed)}")
    print(f"covers nothing {len(empty)}")

    if subsumed:
        print()
        print("subsumed — every line also covered by the named test:")
        for name, other in subsumed:
            print(f"    {name}\n        also covered by {other}")
    if empty:
        print()
        print("covers no production line in this module:")
        for name in empty:
            print(f"    {name}")

    # Leave the profiles as a full run so the gate is measurable again.
    clear_profiles(options.module, options.build_dir)
    run([str(binary), "--gtest_brief=1"])
    return 1 if subsumed or empty else 0


if __name__ == "__main__":
    sys.exit(main())
