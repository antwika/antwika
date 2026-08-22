import re
import subprocess
from pathlib import Path

PATTERN = re.compile(r"\b(if|else|switch|case)\b")


def count(rev, path):
    if rev is None:
        try:
            text = Path(path).read_text()
        except FileNotFoundError:
            return None
    else:
        out = subprocess.run(["git", "show", "%s:%s" % (rev, path)],
                             capture_output=True, text=True)
        if out.returncode != 0:
            return None
        text = out.stdout
    return sum(len(PATTERN.findall(line)) for line in text.split("\n"))


FILES = [
    "src/apps/editor/src/ui/EditorBindings.cpp",
    "src/apps/editor/src/ui/MenuBar.cpp",
    "src/apps/editor/src/editor/draw/EditorHints.cpp",
    "src/apps/editor/src/editor/panels/EditorMenus.cpp",
    "src/apps/editor/src/editor/panels/EditorDialogs.cpp",
    "src/libs/voxel/src/VoxelCube.cpp",
    "src/libs/tile/src/TileShapes.cpp",
    "src/libs/solver/src/VoxelWeaveWants.cpp",
    "src/libs/decor/src/DecorWeave.cpp",
    "src/libs/input/src/DirectionKeys.cpp",
]

base = "cadf3db2"
was_total = 0
now_total = 0
print("%-56s %6s %6s" % ("file", "before", "after"))
for path in FILES:
    was = count(base, path)
    now = count(None, path)
    was_total += was
    now_total += now
    print("%-56s %6d %6d" % (path.split("src/")[-1], was, now))
print("%-56s %6d %6d" % ("TOTAL", was_total, now_total))
