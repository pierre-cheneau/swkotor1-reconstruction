"""Refresh the source hashes that build-manifest.json pins.

    uv run pin_sources.py --check           report pinned files whose hash differs
    uv run pin_sources.py                   re-pin every changed file
    uv run pin_sources.py NEW_FILE...       also pin new files (relative paths)

build.py refuses to build when a pinned file changed, so a source change is
reviewed together with its new hash. New files are placed after the last
pinned file of the same directory. A pinned file that no longer exists is an
error: remove it from the manifest's files and units by hand.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "build-manifest.json"


def digest(relative: str) -> str:
    return hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()


def write_manifest(manifest: dict) -> None:
    # The committed manifest is two-space JSON with CRLF line endings.
    MANIFEST.write_bytes((json.dumps(manifest, indent=2) + "\n").replace("\n", "\r\n").encode("utf-8"))


def pin(files: list[dict[str, str]], new: list[str]) -> tuple[list[str], list[str]]:
    """Update files in place; returns (changed, added) paths."""
    missing = [entry["path"] for entry in files if not (ROOT / entry["path"]).is_file()]
    if missing:
        raise SystemExit("pinned files are missing: " + ", ".join(missing))
    changed = []
    for entry in files:
        current = digest(entry["path"])
        if current != entry["sha256"]:
            entry["sha256"] = current
            changed.append(entry["path"])
    pinned = {entry["path"] for entry in files}
    added = []
    for relative in dict.fromkeys(new):
        if relative in pinned:
            continue
        parent = PurePosixPath(relative).parent
        siblings = [i for i, entry in enumerate(files) if PurePosixPath(entry["path"]).parent == parent]
        files.insert(siblings[-1] + 1 if siblings else len(files), {"path": relative, "sha256": digest(relative)})
        added.append(relative)
    return changed, added


def relative_source(argument: str) -> str:
    path = (Path.cwd() / argument).resolve()
    if not path.is_file() or not path.is_relative_to(ROOT):
        raise SystemExit(f"not a file inside the checkout: {argument}")
    return path.relative_to(ROOT).as_posix()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("new", nargs="*", help="files to add to the pinned list")
    parser.add_argument("--check", action="store_true", help="only report; exit 1 if anything would change")
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_bytes())
    files = manifest["targets"]["game"]["files"]
    changed, added = pin(files, [relative_source(a) for a in args.new])
    for path in changed:
        print(f"{'differs' if args.check else 're-pinned'}: {path}")
    for path in added:
        print(f"{'unpinned' if args.check else 'pinned'}: {path}")
    if args.check:
        return 1 if changed or added else 0
    if changed or added:
        write_manifest(manifest)
    print(f"{len(changed)} re-pinned, {len(added)} added, {len(files)} files pinned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
