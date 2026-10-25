# Contributing

## Report a bug

[Open a bug report](https://github.com/mattlgroff/swkotor1-reconstruction/issues/new?template=bug_report.md) with a clear title and:

- What happened and what you expected.
- The location, quest or cutscene, and the actions immediately before the problem.
- Steps to reproduce it and whether it happens every time.
- The project commit or build, Windows version and GPU; relevant logs are helpful.

If useful, attach a ZIP of the save folder from immediately before the bug to the issue. This is optional. Review saves and logs for personal information before posting. Do not commit saves to the source tree or upload original game assets, executables, middleware, secrets or raw memory dumps.

## Source changes

Keep changes focused. Explain the problem, why the change addresses it and how you tested it. Use [BUILDING.md](BUILDING.md) to build with your own purchased game files. State which gameplay paths you checked and any remaining limitations.

The Python tooling (build, generators, matching tools) has tests that need no game files: run `uv run pytest` and `uv run ruff check`. Golden tests in `tests/` lock every generated output and build command line; a change that alters one must say why and regenerate the golden file. `build.py` refuses sources whose hash differs from `build-manifest.json`, so after changing a pinned file run `uv run pin_sources.py` (name new files as arguments); `uv run pin_sources.py --check` only reports. `src/bridge/` is generated: after editing `src/bridge/bridge.json` or a matched source it uses, run `uv run bridge_sources.py`, which also updates the bridge's manifest units and pins.

Contribute source and documentation only. Keep game resources, generated original data, build outputs, saves, original-derived test fixtures and diagnostic harnesses outside the repository; `tests/` uses synthetic stand-ins only. The [license scope](README.md#rights-and-affiliation) does not grant rights to third-party material.
