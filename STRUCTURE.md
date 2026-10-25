# Project structure: one source, two builds

This project reconstructs the GOG Windows 1.03 `swkotor.exe` in two ways
that check each other:

| | Playable build | Matching build |
| --- | --- | --- |
| Work of | [mattlgroff](https://github.com/mattlgroff) | [RayesDiyab](https://github.com/RayesDiyab) |
| Goal | The game runs and can be modded | The source is the original, byte for byte |
| Toolchain | VS2022 MSVC + clang-cl, C++17 | Visual Studio .NET 2002 (MSVC 7.0, `cl` 13.00.9466), as named in the exe's Rich header |
| Where | `src/`, `build.py`, `build-manifest.json` | `matching/` |
| Proof | Manual play (see [STATUS.md](STATUS.md)) | Each function compiles to the original's bytes |
| Output | `swkotor-reconstructed.exe` | Per-function comparison now; the identical `swkotor.exe` later |

Today the modern-toolchain build covers 97.5% of the 9,252 functions and
builds the full game:
- 7,339 have their own definition, 12 of them byte-identical matched code (phase 2)
- 1,421 are folded into other functions
- 264 C runtime functions come from VS2022's library

5,252 (56.8%) are byte-identical. The tracker in
the README shows both, clearly separated.

Neither proof covers the other. Matching proves each function exact but
can't show the game works as a whole. Play shows the game works but can't
show any function is exact. The plan is to move functions from the playable
build to matched source one at a time, without ever breaking the playable
build.

## Credit

The modern-toolchain decompilation (`src/`, `build.py`, the generators, the
build manifest and STATUS.md) is mattlgroff's work. The byte-identical
decompilation (`matching/`: the matched sources and headers, the tools,
the LIBCMT and symbol-map findings, and the tracker) is RayesDiyab's work.
Both are co-maintained here. Contributions keep their authors' credit
through git history.

## Layout

```
src/, build.py, build-manifest.json, local_generators/   playable build (unchanged)
pin_sources.py            refresh the source hashes build-manifest.json pins
bridge_sources.py         generate src/bridge/ (matched code in the playable build) from src/bridge/bridge.json
cpp_scope.py              namespace-scope C++ declarations, for binding names across units
bridge_gate.py            emulate each bridged function against the original on a finished build (local)
tests/                    tooling tests and golden outputs, on synthetic game files only
matching/
  src/<bucket>/*.cpp        matched sources; buckets are address ranges, see below
  include/<bucket>/*.h      2002-style class declarations at original offsets
  status.csv                result per annotated function (MATCH / NONMATCH)
  functions.csv             the exe's 20,397 function starts; kind "eh" = EH funclet
  crt_libcmt.csv            functions identical to the stock LIBCMT.LIB
  symbols.csv               symbol -> original address, the bridge (phase 2)
  mac_names.csv             Mac PowerPC name hints and source files by address
  link_order.csv            inferred original link order (source file runs)
  assets/progress.svg       progress tracker, refreshed by CI
  tools/matchcmp.py         compile with MSVC 7.0 and compare each function
  tools/libmatch.py         find stock library functions in the exe
  tools/symbolmap.py        build symbols.csv from matched objects
  tools/ppcnames.py         pair functions with the Mac PowerPC builds' names
  tools/test_ppcnames.py    its regression tests, on synthetic inputs
  tools/render_banner.py    render the tracker
.github/workflows/update-progress-tracker.yml
.github/workflows/tooling.yml                  lint, pins and tooling tests
```

Buckets `a`..`n` are address ranges of the exe. They let parallel work
proceed without touching the same files, and say nothing about subsystems.

## Ownership

- **`src/` and the playable build** stay as they are until phase 2 is
  agreed. Matching changes do not edit `src/`.
- **`matching/`** holds matching work. A matching change adds or edits files
  there and regenerates `status.csv`. CI then redraws the tracker.
- Both follow [CONTRIBUTING.md](CONTRIBUTING.md): source and docs only, no
  original executables, assets, compiler binaries or generated original data.
  The tools take the user's own `swkotor.exe` and MSVC 7.0 install through
  `--exe`/`--msvc` or `KOTOR_ORIGINAL_EXE`/`KOTOR_MSVC7`.

## Phases

### 1. Side by side (this change)

The matching track lives next to the playable build. Today:
- **5,252** functions are byte-identical and **504** are attempts not yet
  identical, out of about 9,250 functions. EH funclets are excluded; the
  compiler emits those from their parent functions.
- **The C runtime is the stock VS.NET 2002 `LIBCMT.LIB`.** In the CRT band,
  395 of 447 functions (97% of the bytes) are identical to that library.
  Those need no source: the matching build links the library, as the
  original did. The tracker shows them as "stock LIBCMT".

### 2. The bridge: matched code inside the playable build

Every playable-build function is named after its original address
(`kotor_fastcall_005b2ef0`, `kotor_cdecl_...`). Matched code uses the
original C++ names (`CSWBaseItem::CSWBaseItem`). `matching/tools/symbolmap.py`
connects the two automatically. Each relocation in a byte-identical
function sits where the original holds the real address, so reading the
original there tells you which address each symbol stands for.
`matching/symbols.csv` currently maps 9,510 symbols:
- 5,232 functions defined by matched code
- 1,962 functions it calls
- 2,316 data symbols: globals, vtables and strings

`bridge_sources.py` implements the first step. `src/bridge/bridge.json` names
matched functions (status MATCH) and, for each, the playable site it now
serves. The tool writes one unit per matched file into `src/bridge/`:
- the matched file's own declarations and the selected functions only, with
  its `matching/include` headers inlined, compiled by MSVC 14.44 under the
  manifest's strict recipe (`/W4 /WX`). Only two warnings that byte-identical
  code raises by design are silenced, around the matched code alone: an
  unused parameter and a narrowing conversion, both the original's own;
- all inside a namespace of the unit's own, so no class, inline function or
  vtable can merge with another unit's copy;
- one `extern "C"` entry per function, `kotor_fastcall_<address>`, taking
  classes as `void*` like the rest of the playable build. A class returned by
  value is built at the hidden pointer the caller passes after EDX, which the
  entry returns: the original's frame. A class passed by value stays one;
- forwards for the callees the spec names, below;
- a data bridge for the globals the slice uses, below.

A slice keeps only what its entries reach. Explicit instantiations go: the
matched file has them so the original's link emits template members, while an
entry instantiates exactly the members it calls. An unannotated helper of the
matched file becomes `inline`, so it is emitted only if used. Any other
function or global the slice uses stays unresolved and fails the link.

A forward defines a callee's member as a naked jump to a playable entry for the
callee's address: a bridged function's generated entry, `kotor_fastcall_<address>`
in `src/`, or the `extern "C"` function the spec names. A thiscall member and its
fastcall entry share one frame (ECX this, EDX unused, the same stack arguments
and cleanup), so one jump serves members, operators, constructors, destructors
and by-value returns alike. `cpp_scope.py` finds the entry, block-form
`extern "C" { }` included, and the generator checks that it pops the original's
stack bytes and returns in its register. When `symbols.csv` names an address
only under folded copies (`~CExoString` shares 005E5C20 with two other
destructors), the spec gives the signature, which the slice must declare.
MSVC still emits a constructor's table pointer and member construction ahead
of a naked body, with no frame, so a constructor forward is refused unless the
class has no table, no bases and no member needing construction. The same
holds for a destructor's table pointer.

The data bridge binds a matched global to the playable build's storage for the
same original address. `symbols.csv` gives the global's address.
`cpp_scope.py` reads the pinned playable sources for the one external,
non-reference variable named after that address, with a type spelled in
builtin or `<cstdint>` types. Storage the build generates rather than `src/`
defines, such as `game_prepare.py`'s cells (`g_pRules` is one), counts as
defined. The generator rewrites the matched `extern`
declaration as a reference to that storage. The reference is cast without a
function call, so it is initialized statically. A `static_assert` beside it
checks the storage is large enough. The link then checks the identity: MSVC
encodes namespace and type into the symbol name.

Read-only data (`.rdata`) the playable build keeps only in unit-local
constants is copied instead, and only when every playable definition of the
address agrees on its literal value. The copy is `const`, whatever the matched
declaration says, since the original cannot write `.rdata`. Vtables are
globals too: a bridged constructor or destructor re-points the object at the
playable build's table for the class's original vftable address. Its entry
asserts the class is polymorphic, or, without a table, that it is not. The
class is declared `__declspec(novtable)` in the slice, which then needs no table
of its own, nor the virtual functions it would name; the generator refuses
this if anything else in the slice constructs the class. Each generated unit
lists what it bound and what it could not, with the reason.

The tool also keeps the manifest's bridge units and pins in step.
`--check` and a test fail if anything is out of date. The playable site then
calls the entry instead of its own copy of the logic. The tracker counts
the entries as definitions and ignores the addresses that the inlined
matched code mentions.

Forty functions now serve playable sites:
- Native port bodies whose logic was the callee's, inline:
  - `CResRef::GetLength`, `CSWSCreature::SetFlag288Bit3`,
    `CSWRoomSurfaceMesh::GetAdjacentSecond`, `CExoInput::GetLastSetMouseDownState`
  - `CSWInventory`'s constructor (bound to the playable inventory vtable),
    `GetItemSlotPtr` and `PutItemInSlot`
  - `CSWSItem::ApplyItemProperties` (through `CServerAIMaster::OnItemPropertyApplied`)
    and `InitRepository` (with `CItemRepository`'s constructor, which the
    original inlines), on the game heap
  - `CClientExoApp`'s `GetPlayModulePanel`, `EmptyHitcheckIgnoreList`,
    `GetPlayerCharacterSet`, `GetLoadFromIFO`, and `ServerModeChange` and
    `GetPlayerCharacterName` through their `CClientExoAppInternal` members
  - `CServerExoAppInternal::FUN_004b4030` and `GetItemByGameObjectID`, and
    `CServerExoApp::GetModulePreviouslyVisited`
  - `CExoLocString::GetStringLength` through `CExoLocStringInternal`'s
  - `CExoLinkedList<CExoSoundSourceInternal>::Contains`, at the three calls the
    original makes to it
  - `CSWSDoor::GetMeshOutline`, its two `Vector`s by value
- Logic inline in `derived/` code, now behind a port of its own:
  - `CSWSCreatureStats::GetTotal{STR,DEX,CON,INT,WIS,CHA}Bonus`
  - `CSWGlobalVariableTable::Add{Boolean,Number,Location,String}`
  - the `CExoLinkedList` constructors of the AI event queue and the combat
    round's scheduled actions
  - `CExoArrayList<CSWSTagNode>::Insert`, its 40-byte node by value
  - `CServerAIMaster::AppendToScriptsRun`, whose `CExoString` constructor,
    `operator+`, `operator=` and destructor forward to playable entries

`bridge_gate.py` checks a finished build, locally, like the L1 math gate.
It uses the Unicorn emulator, never running either program natively, to
execute each original function and its rebuilt entry on identical random
state and compare:
- return values, memory writes and stack cleanup;
- the bound globals, with each one's value set at both programs' addresses;
- the calls made to forwards and the game heap, answered identically on both
  sides, with stack temporaries compared by slot and literals by their text;
- vtable pointers, mapped between the two programs.

It derives everything from the spec, the bridge's bindings and the build's
link map, and checks each forward's cleanup against the original callee's
`ret`. SEH frames run on an emulated thread block. It fails a function whose
trials rarely run to the end, so a pass cannot be vacuous; a trial that uses
up its steps is reported and not compared. All forty functions pass. Its tests
use hand-assembled functions: equivalent pairs agree, and mutants (a wrong
field, a missing stack pop, a wrong `this` or argument for a forward, a wrong
literal, a misbound global, an unmapped vtable) are caught. The functions still
need play-testing.

The bridge refuses unsound bindings rather than guessing. `AurSetFog` writes
`g_vFogColor[3]` as one array, but the playable build keeps those three floats
as separate variables, so it stays unbridged.

Of the matched functions playable code reaches, these stay unbridged:
- Ported, but under names without their address, which must not be replaced:
  `CExoRandInternal::Rand` and `GenSeed` (exact naked assembly),
  `CExoKeyTable::AllocateTable` and `BuildNewTable`,
  `CExoSoundInternal::MuteSound` and `UnMuteSound`, `CExoAliasList::Add` and
  the `CExoAlias` constructor.
- Out of scope (developer console, debug item menu): `ConsoleExecute`,
  `ConsoleComplete`, `CGuiInGame::ShowSubItemCreateMenu`.
- Reached only from the Neverwinter remote-administration handlers
  (00528380, 00579920, 00579A80): `CServerExoApp::GetModuleName`,
  `GetPlayerListString` and `LoadModule`.
- `CServerExoApp::SaveGame` and `LoadGame`: their matched declarations take 3
  and 2 parameters, but the callees pop 16 bytes, which their tail jumps hide
  in the matching. The declarations need fixing first.
- `CServerExoApp::RunModule`: a two-instruction thunk to a playable function,
  whose port family passes the internal object; swapping it would re-plumb 20
  files.
- The `CSWDoorSurfaceMesh` constructor: its base constructor is in another
  matched file and constructs members first, so no forward can stand in for it.

Still open: 110 symbols resolve to more than one address. These are identical
copies in the original, such as per-object string literals and duplicated
inline bodies, or naming slips in matched headers. They need a rule.

### 3. Replace

When every function in a playable unit is matched, bridged and play-tested,
the unit's playable version can be retired, and the matched source becomes
the only source for that code.

### 4. Whole executable

Beyond functions: data, strings and vtables in source; the original link
order; linking `LIBCMT.LIB`; and comparing the whole linked `swkotor.exe`
with the original.

## Working on the matching track

See [matching/README.md](matching/README.md) for the annotation format, how
to run the tools, and the source-shape patterns that make functions match.

## Reviewing this change

1. **Scope:** `git diff main --stat` touches only `matching/`,
   `STRUCTURE.md`, `.github/workflows/update-progress-tracker.yml` and four
   lines of `README.md`. Nothing in `src/`, `build.py` or the build
   manifest changes, so the playable build is unaffected.
2. **Reproduce the numbers** (needs MSVC 7.0 and your exe; see
   [matching/CONTRIBUTING.md](matching/CONTRIBUTING.md)):
   ```powershell
   uv run .\matching\tools\matchcmp.py -q matching\src                 # 5,252 / 5,756
   uv run .\matching\tools\libmatch.py "<msvc7>\LIB\LIBCMT.LIB" "<swkotor.exe>" out.csv
   uv run .\matching\tools\render_banner.py                            # tracker, no compiler needed
   ```
3. **Contributing afterwards:** people and AI agents follow
   [matching/CONTRIBUTING.md](matching/CONTRIBUTING.md).
