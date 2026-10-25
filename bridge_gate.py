"""Check the bridged functions against the original: run both on identical state and compare everything.

    uv run bridge_gate.py --game-dir GAME --build BUILD_OUT [--trials N] [--only ADDRESS ...]

A local check of a finished build, like the L1 math gate. It reads the user's own
swkotor.exe (the supported GOG release) and BUILD_OUT's
swkotor-reconstructed.exe and link.map, and runs nothing natively. For each
function in src/bridge/bridge.json it emulates (Unicorn) the original bytes and
the rebuilt kotor_fastcall_<address> entry on identical, randomly built state:
  memory     a sandbox of words that are mostly pointers into itself, small
             integers, and objects whose vtables lead to recording stubs;
  inputs     this, the arguments, and every global the bridge binds: the same
             value at the original address and at the rebuilt symbol;
  callees    each forward stops at its entry on both sides, where the call is
             recorded and answered identically.
Each trial compares:
  - the return value and the stack cleanup;
  - every sandbox byte, with vtable pointers mapped from the playable tables to
    the original vftables;
  - the bound globals;
  - the calls made.
A function passes when every trial agrees and enough of them run to the end. A
fault agrees only with a fault. Exit 0 when every function passes.
"""
from __future__ import annotations

import argparse
import random
import re
import struct
import sys
import zlib
from dataclasses import dataclass
from pathlib import Path

import pefile
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32, Uc, UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EIP, UC_X86_REG_ESP

import bridge_sources
from local_generators import original

SANDBOX, SIZE = 0x20000000, 0x20000
THIS_RANGE = 0x2000                  # 'this' lies near the start, so fields up to SIZE - THIS_RANGE stay mapped
STACK, STACK_SIZE = 0x30000000, 0x10000
RETURN = 0x3F000000                  # the caller's return address: reaching it ends a run
STUBS, STUB_COUNT = 0x3E000000, 64   # virtual-call targets, 16 bytes apart
OBJECTS, OBJECT_SIZE = 32, 0x40      # object slots at the sandbox's end; word 0 is a vtable of stubs
WORLDS = 24                          # sandboxes per function; trials vary this, arguments and globals over them
COMPLETED = 0.1                      # the share of trials that must run to the end on both sides
STEPS = 200_000


@dataclass(frozen=True)
class Image:
    base: int
    data: bytes

    @classmethod
    def load(cls, data: bytes) -> Image:
        pe = pefile.PE(data=data, fast_load=True)
        return cls(pe.OPTIONAL_HEADER.ImageBase, bytes(pe.get_memory_mapped_image()))

    @property
    def end(self) -> int:
        return self.base + len(self.data)


@dataclass(frozen=True)
class Callee:
    """A forward: stopped at its entry on both sides, recorded under its original address."""
    original: int
    rebuilt: int
    popped: int                      # stack bytes it removes on return


@dataclass(frozen=True)
class Target:
    name: str
    original: int
    rebuilt: int
    params: tuple[str, ...]          # parameter types, to choose arguments
    popped: int                      # stack bytes the entry removes
    mask: int | None                 # return-value bits compared; None for void
    callees: tuple[Callee, ...] = ()
    globals: tuple[tuple[int, int], ...] = ()     # (original address, rebuilt address)
    vtables: tuple[tuple[int, int], ...] = ()     # (original vftable, rebuilt playable table)


@dataclass(frozen=True)
class Report:
    target: Target
    trials: int
    agreed: int
    completed: int                   # trials that ran to the end on both sides
    difference: str | None

    @property
    def passed(self) -> bool:
        return self.agreed == self.trials and self.completed >= COMPLETED * self.trials


# ---------------------------------------------------------------- the build's symbols

def link_symbols(link_map: Path) -> dict[str, int]:
    """Public symbol -> address (Rva+Base) from a link.map."""
    found = {}
    for line in link_map.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s", line)
        if m:
            found[m.group(1)] = int(m.group(2), 16)
    return found


def resolve(symbols: dict[str, int], prefix: str, exact: bool = False) -> tuple[str, int]:
    """The one symbol named prefix (exact) or starting with it."""
    found = [(n, a) for n, a in symbols.items() if (n == prefix if exact else n.startswith(prefix))]
    if len(found) != 1:
        raise ValueError(f"{len(found)} link.map symbols match {prefix}")
    return found[0]


def storage_symbol(symbols: dict[str, int], storage: bridge_sources.Storage) -> int:
    """The rebuilt address of a bound playable variable, found by its MSVC name."""
    v = storage.variable
    if v.statement.c_linkage:
        return resolve(symbols, f"_{v.name}", exact=True)[1]
    scopes = "@".join(reversed(v.statement.namespace))
    return resolve(symbols, f"?{v.name}@{scopes}@@3" if scopes else f"?{v.name}@@3")[1]


def entry(symbols: dict[str, int], address: int) -> tuple[int, int]:
    """(rebuilt address, stack bytes popped) of kotor_fastcall_<address>: @N counts ECX and EDX."""
    name, rebuilt = resolve(symbols, f"@kotor_fastcall_{address:08x}@")
    return rebuilt, int(name.rsplit("@", 1)[1]) - 8


def return_mask(type_: str) -> int | None:
    if type_ == "void":
        return None
    size = 4 if bridge_sources.is_object(type_) or type_.endswith("*") else bridge_sources.size_of(type_, "")
    if size not in {1, 2, 4}:
        raise ValueError(f"return type {type_} is not compared (EAX only)")
    return (1 << (8 * size)) - 1


def targets(symbols: dict[str, int], only: set[int] | None = None) -> list[Target]:
    """Every bridged function, with its rebuilt entry, forwards, bound globals and vtables."""
    found = []
    for unit in bridge_sources.units():
        plan = unit.plan
        callees = tuple(Callee(f.address, *entry(symbols, f.address)) for f in plan.forwards)
        globals_ = tuple((s.address, storage_symbol(symbols, s)) for s in plan.globals)
        for f in unit.functions:
            if only and f.address not in only:
                continue
            table = plan.tables.get(f.address)
            rebuilt, popped = entry(symbols, f.address)
            found.append(Target(f"{f.owner}::{f.name}", f.address, rebuilt, f.params, popped,
                                return_mask(f.returns), callees, globals_,
                                ((table.address, storage_symbol(symbols, table)),) if table else ()))
    return sorted(found, key=lambda t: t.original)


# ---------------------------------------------------------------- state

class World:
    """One random sandbox, identical for both runs."""

    def __init__(self, rng: random.Random, images: tuple[Image, ...]) -> None:
        self.images = images
        words = [self.word(rng) for _ in range(SIZE // 4)]
        stubs = SIZE - OBJECTS * OBJECT_SIZE - STUB_COUNT * 4          # a vtable whose slots are the stubs
        words[stubs // 4:stubs // 4 + STUB_COUNT] = [STUBS + 16 * k for k in range(STUB_COUNT)]
        self.objects = [SANDBOX + SIZE - OBJECTS * OBJECT_SIZE + OBJECT_SIZE * k for k in range(OBJECTS)]
        for o in self.objects:
            first = (o - SANDBOX) // 4
            words[first] = SANDBOX + stubs
            words[first + 1:first + OBJECT_SIZE // 4] = [rng.choice([rng.randrange(9), self.pointer(rng)])
                                                         for _ in range(OBJECT_SIZE // 4 - 1)]
        self.data = struct.pack(f"<{len(words)}I", *words)

    def word(self, rng: random.Random) -> int:
        roll = rng.random()
        if roll < 0.6:
            return self.pointer(rng)
        if roll < 0.75:
            return SANDBOX + SIZE - OBJECTS * OBJECT_SIZE + OBJECT_SIZE * rng.randrange(OBJECTS)
        return rng.randrange(9) if roll < 0.9 else self.outside(rng.getrandbits(32))

    @staticmethod
    def pointer(rng: random.Random) -> int:
        return SANDBOX + 4 * rng.randrange((SIZE - OBJECTS * OBJECT_SIZE) // 4 - STUB_COUNT)

    def outside(self, value: int) -> int:
        """A word that is no address inside either image, whose ranges differ between the two runs."""
        while any(i.base <= value < i.end for i in self.images):
            value = (value + 0x10000000) & 0xFFFFFFFF
        return value

    def argument(self, rng: random.Random, type_: str) -> int:
        if type_.endswith(("*", "&")):
            return rng.choice([self.pointer(rng), rng.choice(self.objects)])
        value = rng.choice([rng.randrange(9), 1 << rng.randrange(32), rng.randrange(64), 0xFFFFFFFF,
                            self.outside(rng.getrandbits(32))])
        size = bridge_sources.size_of(type_, "") or 4
        return value & ((1 << (8 * min(size, 4))) - 1)


@dataclass(frozen=True)
class Inputs:
    world: World
    this: int
    args: tuple[int, ...]
    globals: tuple[int, ...]         # one value per target global


def answer(record: tuple, world: World) -> tuple[int, int]:
    """The same deterministic reply to the same call on both sides: (return value, value for out-parameters).
    Returns lean on 0 and 1, the usual statuses; out-parameters always receive an object."""
    h = zlib.crc32(repr(record).encode())
    returned = (0, 0, 1, world.objects[(h >> 4) % OBJECTS])[h % 4]
    return returned, world.objects[(h >> 12) % OBJECTS]


# ---------------------------------------------------------------- emulation

def run(image: Image, start: int, target: Target, inputs: Inputs, rebuilt: bool) -> tuple | None:
    """Everything observable after one run, or None when it faults."""
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(image.base, (len(image.data) + 0xFFF) & ~0xFFF)
    uc.mem_write(image.base, image.data)
    for base, size in ((SANDBOX, SIZE), (STACK, STACK_SIZE), (RETURN, 0x1000), (STUBS, 0x1000)):
        uc.mem_map(base, size)
    uc.mem_write(SANDBOX, inputs.world.data)
    addresses = [mapped if rebuilt else address for address, mapped in target.globals]
    for address, value in zip(addresses, inputs.globals):
        uc.mem_write(address, struct.pack("<I", value))
    esp = STACK + STACK_SIZE - 0x100
    uc.mem_write(esp, struct.pack(f"<I{len(inputs.args)}I", RETURN, *inputs.args))
    uc.reg_write(UC_X86_REG_ESP, esp)
    uc.reg_write(UC_X86_REG_ECX, inputs.this)
    uc.reg_write(UC_X86_REG_EDX, 0xDEADBEEF)
    callees = {(c.rebuilt if rebuilt else c.original): c for c in target.callees}
    calls: list[tuple] = []

    def stop(uc_: Uc, address: int, _size: int, _data: object) -> None:
        callee = callees.get(address)
        if callee is None and not STUBS <= address < STUBS + 16 * STUB_COUNT:
            return
        sp = uc_.reg_read(UC_X86_REG_ESP)
        popped = callee.popped if callee else 0
        args = struct.unpack(f"<{popped // 4}I", uc_.mem_read(sp + 4, popped)) if popped else ()
        record = (callee.original if callee else address, uc_.reg_read(UC_X86_REG_ECX),
                  tuple("stack" if STACK <= a < STACK + STACK_SIZE else a for a in args))
        calls.append(record)
        returned, out = answer(record, inputs.world)
        for a in args:                                   # out-parameters in the caller's frame
            if STACK <= a < STACK + STACK_SIZE - 4:
                uc_.mem_write(a, struct.pack("<I", out))
        uc_.reg_write(UC_X86_REG_EAX, returned)
        uc_.reg_write(UC_X86_REG_ESP, sp + 4 + popped)
        uc_.reg_write(UC_X86_REG_EIP, struct.unpack("<I", uc_.mem_read(sp, 4))[0])

    uc.hook_add(UC_HOOK_CODE, stop)
    try:
        uc.emu_start(start, RETURN, count=STEPS)
    except UcError:
        return None
    if uc.reg_read(UC_X86_REG_EIP) != RETURN:
        return None                                      # ran out of steps: treated like a fault
    sandbox = bytearray(uc.mem_read(SANDBOX, SIZE))
    if rebuilt and target.vtables:
        tables = {rebuilt_table: original_table for original_table, rebuilt_table in target.vtables}
        for k in range(0, SIZE, 4):
            value = struct.unpack_from("<I", sandbox, k)[0]
            if value in tables:
                struct.pack_into("<I", sandbox, k, tables[value])
    eax = uc.reg_read(UC_X86_REG_EAX)
    return ((eax & target.mask) if target.mask is not None else None, uc.reg_read(UC_X86_REG_ESP) - esp,
            bytes(sandbox), tuple(calls), tuple(struct.unpack("<I", uc.mem_read(a, 4))[0] for a in addresses))


def check(target: Target, original_image: Image, rebuilt_image: Image, trials: int) -> Report:
    rng = random.Random(target.original)
    worlds = [World(rng, (original_image, rebuilt_image)) for _ in range(min(WORLDS, trials))]
    agreed = completed = 0
    difference = None
    for _ in range(trials):
        world = rng.choice(worlds)
        inputs = Inputs(world, SANDBOX + 4 * rng.randrange(THIS_RANGE // 4),
                        tuple(world.argument(rng, t) for t in target.params),
                        tuple(rng.choice([world.pointer(rng), rng.choice(world.objects)]) for _ in target.globals))
        a = run(original_image, target.original, target, inputs, rebuilt=False)
        b = run(rebuilt_image, target.rebuilt, target, inputs, rebuilt=True)
        if a == b:
            agreed += 1
            completed += a is not None
        elif difference is None:
            difference = describe(a, b, inputs)
    return Report(target, trials, agreed, completed, difference)


def describe(a: tuple | None, b: tuple | None, inputs: Inputs) -> str:
    if a is None or b is None:
        side = "original" if a is None else "rebuilt"
        return f"only the {side} run faulted (this={inputs.this:08X} args={inputs.args})"
    parts = ("return value", "stack cleanup", "memory", "calls", "globals")
    differing = [name for name, x, y in zip(parts, a, b) if x != y]
    detail = ""
    if "memory" in differing:
        k = next(i for i in range(SIZE) if a[2][i] != b[2][i])
        detail = f"; first byte {SANDBOX + k:08X}: {a[2][k]:02X} vs {b[2][k]:02X}"
    elif differing:
        name = differing[0]
        detail = f"; {a[parts.index(name)]} vs {b[parts.index(name)]}"
    return f"{', '.join(differing)} differ (this={inputs.this:08X} args={inputs.args}){detail}"


# ---------------------------------------------------------------- command line

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--game-dir", type=Path, required=True, help="the installation holding swkotor.exe")
    parser.add_argument("--build", type=Path, required=True, help="a build.py output directory")
    parser.add_argument("--trials", type=int, default=300, help="trials per function (default 300)")
    parser.add_argument("--only", nargs="+", help="original addresses to check, e.g. 004B1740")
    args = parser.parse_args()
    original_image = Image.load(original.OriginalImage(args.game_dir / "swkotor.exe").data)
    rebuilt_image = Image.load((args.build / "swkotor-reconstructed.exe").read_bytes())
    only = {int(a, 16) for a in args.only} if args.only else None
    reports = [check(t, original_image, rebuilt_image, args.trials)
               for t in targets(link_symbols(args.build / "link.map"), only)]
    for r in reports:
        verdict = "PASS" if r.passed else "FAIL" if r.agreed != r.trials else "INCONCLUSIVE"
        print(f"{verdict:12} {r.target.original:08X} {r.target.name}: {r.agreed}/{r.trials} agree, "
              f"{r.completed} ran to the end" + (f"; {r.difference}" if r.difference else ""))
    failed = [r for r in reports if not r.passed]
    print(f"BRIDGE GATE {'PASS' if not failed else 'FAIL'}: {len(reports) - len(failed)}/{len(reports)} functions")
    return 1 if failed or not reports else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError) as exc:
        print(f"Bridge gate stopped: {exc}", file=sys.stderr)
        sys.exit(1)
