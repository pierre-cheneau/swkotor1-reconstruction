"""Generate private build inputs from a separately purchased, hash-pinned GOG game.

Outputs contain original game material. Keep the complete output directory local
and ignored; never add it to a source snapshot or upload it. This tool does not
grant redistribution rights. It never executes the game or its DLLs.
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.dont_write_bytecode = True              # keep the reviewed generator tree free of caches
sys.path.insert(0, str(ROOT))

import pefile  # noqa: E402  (installed separately; checked before any output exists)

from local_generators import (  # noqa: E402  # noqa: E402
    controller,
    diagnostic_literals,
    game_prepare,
    imports,
    literals,
    pow_constants,
    renderer,
    resources,
    rules,
    sound,
    warning_locale,
)
from local_generators.original import EXE_SHA256, SPECS, OriginalImage, sha256, write_text  # noqa: E402

# Output directory -> (spec, generated header name) of the renderer data groups.
RENDERER_GROUPS = {
    "mesh": ("mesh-spec.json", renderer.HEADER),
    "vp-data": ("vp-data-spec.json", renderer.HEADER),
    "vp-rdata": ("vp-rdata-spec.json", renderer.HEADER),
    "epoch": ("epoch-spec.json", "kotor_renderer_programs_original_data.generated.h"),
    "window": ("window-spec.json", "kotor_window_original_data.generated.h"),
}
OUTPUT_DIRECTORIES = ("resources", "renderer", "game-prepare", "sound", "rules", "imports", "resource-pairs",
                      "controller", "texture", "upgrade", "class-choice", "pow", "diagnostics", *RENDERER_GROUPS)
SCOPE = ("Resources, GL/mesh/VP/epoch/window/controller renderer data, resource-pair descriptors, "
         "texture/upgrade data, preparation data, sound data, optional rules data/import libraries; "
         "not full dependency closure")


def disjoint(a: Path, b: Path) -> bool:
    return not (a.is_relative_to(b) or b.is_relative_to(a))


def generate(game: Path, out: Path, rules_src: Path | None, renderer_spec: Path, librarian: Path | None) -> None:
    executable = (game / "swkotor.exe").resolve(strict=True)
    if not game.is_dir() or not executable.is_relative_to(game):
        raise ValueError("The original executable does not match the supported GOG release")
    image = OriginalImage(executable)
    if not disjoint(out, game) or not disjoint(out, ROOT):
        raise ValueError("Output must be outside and separate from the installation and source tree")
    if out.exists():
        raise ValueError("Output must be a new directory; existing files are never replaced")
    if rules_src and (not rules_src.is_dir() or not disjoint(out, rules_src)):
        raise ValueError("Rules source must be a separate input directory")
    if not disjoint(out, renderer_spec):
        raise ValueError("Output overlaps an input")
    renderer_specs = {"renderer": (renderer_spec, renderer.HEADER),
                      **{group: (SPECS / spec, header) for group, (spec, header) in RENDERER_GROUPS.items()}}
    for spec, _ in renderer_specs.values():
        renderer.validate(json.loads(spec.read_text(encoding="utf-8")))

    out.mkdir(parents=True)
    (out / ".gitignore").write_bytes(b"*\r\n")
    (out / "NEVER-PUBLISH.txt").write_bytes(b"Original game-derived build inputs: keep this directory private.\r\n")
    for name in OUTPUT_DIRECTORIES:
        (out / name).mkdir()
    resources.original_resources(image, out / "resources" / "swkotor-original.res")
    for group, (spec, header) in renderer_specs.items():
        renderer.extract(image, spec, out / group, header)
    literals.resource_pairs(image, out / "resource-pairs" / "resource_pairs.cpp")
    literals.texture_literal(image, out / "texture" / "texture_literal.cpp")
    literals.upgrade_slots(image, out / "upgrade" / "upgrade_slots.cpp")
    literals.class_choices(image, out / "class-choice" / "class_choices.cpp")
    literals.extra_data(image, out)
    pow_constants.pow_constants(image, out / "pow" / "generated_constants.inc")
    diagnostic_literals.diagnostic_literals(image, out / "diagnostics" / "diagnostic_literals.cpp")
    warning_locale.warning_locale(image, out / "warning-locale")
    controller.controller(image, out / "controller" / "controller_rdata.h")
    game_prepare.game_prepare(image, out / "game-prepare")
    sound.sound(image, out / "sound")
    if rules_src:
        rules.rules(image, rules_src, out / "rules")
    import_receipts = imports.import_libraries(game, out / "imports", librarian) if librarian else []
    image.verify_unchanged()
    receipt = {"exe_sha256": EXE_SHA256, "pefile_version": pefile.__version__,
               "rules_generated": bool(rules_src), "import_libraries": import_receipts, "scope": SCOPE,
               "generators": {p.name: sha256(p.read_bytes()) for p in SPECS.iterdir() if p.is_file()},
               "files": {str(p.relative_to(out)): sha256(p.read_bytes()) for p in out.rglob("*") if p.is_file()}}
    write_text(out / "generation-receipt.json", json.dumps(receipt, indent=2), "\r\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True,
                        help="New private output outside this source tree and game install")
    parser.add_argument("--rules-src", type=Path, help="Reviewed rules source directory, read only")
    parser.add_argument("--renderer-spec", type=Path,
                        help="Reviewed renderer address specification; default is the six-object GL specification")
    parser.add_argument("--lib-exe", type=Path,
                        help="Optional installed MSVC x86 lib.exe; enables middleware import generation")
    args = parser.parse_args()
    generate(args.game_dir.resolve(strict=True), args.out.resolve(),
             args.rules_src.resolve(strict=True) if args.rules_src else None,
             (args.renderer_spec or SPECS / "renderer-spec.json").resolve(strict=True),
             args.lib_exe.resolve(strict=True) if args.lib_exe else None)
    print("Generated private local inputs; do not publish the output directory.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, RuntimeError, OSError, KeyError, subprocess.SubprocessError) as exc:
        print(f"Generation stopped: {exc}", file=sys.stderr)
        sys.exit(1)
