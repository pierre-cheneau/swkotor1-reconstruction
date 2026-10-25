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
             recorded and answered identically; so do the game heap's
             operator new and free (the original's) and the global operator
             new and delete forms (the rebuilt build's);
  thread     FS addresses a thread block with an empty exception list, so
             functions with SEH frames run.
Before running, each forward's rebuilt entry must pop the stack bytes the
original callee's own code pops (its ret, read through thunks).
Each trial compares:
  - the return value and the stack cleanup;
  - every sandbox byte, with vtable pointers mapped from the playable tables to
    the original vftables;
  - the bound globals;
  - the calls made, with stack addresses compared by slot (in order of first
    use: the two frames lay temporaries out differently) and pointers into
    .rdata by the string there (each program has its own literals).
A function passes when every compared trial agrees and enough of them run to
the end. A fault agrees only with a fault. A trial where either side uses up
its steps (a long loop: the rebuilt code's /Ob0 calls cost more) is reported
and not compared. Exit 0 when every function passes.
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
from capstone import CS_ARCH_X86, CS_MODE_32, Cs
from unicorn import UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_32, Uc, UcError
from unicorn.x86_const import (
    UC_X86_REG_DS,
    UC_X86_REG_EAX,
    UC_X86_REG_ECX,
    UC_X86_REG_EDX,
    UC_X86_REG_EIP,
    UC_X86_REG_ES,
    UC_X86_REG_ESP,
    UC_X86_REG_FS,
    UC_X86_REG_GDTR,
    UC_X86_REG_SS,
)

import bridge_sources
from local_generators import original

SANDBOX, SIZE = 0x20000000, 0x20000
THIS_RANGE = 0x2000                  # 'this' lies near the start, so fields up to SIZE - THIS_RANGE stay mapped
STACK, STACK_SIZE = 0x30000000, 0x10000
RETURN = 0x3F000000                  # the caller's return address: reaching it ends a run
STUBS, STUB_COUNT = 0x3E000000, 64   # virtual-call targets, 16 bytes apart
THREAD = 0x3D000000                  # the thread block FS addresses (SEH frames link through fs:[0]); its GDT after
OBJECTS, OBJECT_SIZE = 32, 0x40      # object slots at the sandbox's end; word 0 is a vtable of stubs
WORLDS = 24                          # sandboxes per function; trials vary this, arguments and globals over them
COMPLETED = 0.1                      # the share of trials that must run to the end on both sides
STEPS = 200_000
EXHAUSTED = "exhausted"              # what a run returns when it used up its steps
# The original's operator new (006FA7E6) and free (006FA390, which its operator delete forms reach), and the
# rebuilt build's global forms (src/units/0211 replaces every one): each recorded with its first argument only.
GAME_HEAP = {
    0x006FA7E6: ("??2@YAPAXI@Z", "??_U@YAPAXI@Z"),
    0x006FA390: ("??3@YAXPAX@Z", "??_V@YAXPAX@Z", "??3@YAXPAXI@Z", "??_V@YAXPAXI@Z"),
}


@dataclass(frozen=True)
class Image:
    base: int
    data: bytes
    constants: tuple[tuple[int, int], ...] = ()      # the .rdata ranges, where literals live

    @classmethod
    def load(cls, data: bytes) -> Image:
        pe = pefile.PE(data=data, fast_load=True)
        base = pe.OPTIONAL_HEADER.ImageBase
        rdata = tuple((base + s.VirtualAddress, base + s.VirtualAddress + s.Misc_VirtualSize)
                      for s in pe.sections if s.Name.rstrip(b"\0") == b".rdata")
        return cls(base, bytes(pe.get_memory_mapped_image()), rdata)

    def constant(self, address: int) -> bool:
        return any(start <= address < end for start, end in self.constants)

    @property
    def end(self) -> int:
        return self.base + len(self.data)


@dataclass(frozen=True)
class Callee:
    """A call stopped at its entry on both sides and recorded under its original address: a forward (thiscall,
    ECX and its stack arguments recorded) or a game-heap function (cdecl, its first argument recorded)."""
    original: int
    rebuilt: int
    popped: int                      # stack bytes it removes on return
    recorded: int = -1               # stack words recorded; -1: all it pops, with ECX
    # a heap function's first argument is its size or block, which is all both sides agree on: the rebuilt
    # build's sized delete passes the size as well


@dataclass(frozen=True)
class Target:
    name: str
    original: int
    rebuilt: int
    params: tuple[str, ...]          # the type of each stack word, to choose arguments
    popped: int                      # stack bytes the entry removes
    mask: int | None                 # return-value bits compared; None for void
    callees: tuple[Callee, ...] = ()
    globals: tuple[tuple[int, int], ...] = ()     # (original address, rebuilt address)
    vtables: tuple[tuple[int, int], ...] = ()     # (original vftable, rebuilt playable table)
    defect: str | None = None        # found before running: a forward that does not keep the original's frame


@dataclass(frozen=True)
class Report:
    target: Target
    trials: int
    agreed: int
    completed: int                   # trials that ran to the end on both sides
    difference: str | None
    exhausted: int = 0               # trials not compared: a side used up its steps (a long loop, not a fault)

    @property
    def passed(self) -> bool:
        return self.agreed == self.trials - self.exhausted and self.completed >= COMPLETED * self.trials


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


def entry(symbols: dict[str, int], name: str) -> tuple[int, int]:
    """(rebuilt address, stack bytes popped) of the fastcall entry name: @N counts ECX and EDX."""
    decorated, rebuilt = resolve(symbols, f"@{name}@")
    return rebuilt, int(decorated.rsplit("@", 1)[1]) - 8


def cleanup(image: Image, address: int, depth: int = 4) -> int | None:
    """Stack bytes the original function at address pops: the operand of its first ret, decoded linearly. A thunk
    (a direct jmp before any other control flow, e.g. 'mov ecx,[ecx+4]; jmp X') is followed; a jmp inside a body is
    a branch and is stepped over. None when no ret is found."""
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    code = image.data[address - image.base:address - image.base + 0x4000]
    leading = True                                       # no branch, call or ret decoded yet
    for insn in md.disasm(code, address):
        if insn.mnemonic.startswith("ret"):
            return int(insn.op_str, 0) if insn.op_str else 0
        if insn.mnemonic == "jmp" and leading:
            try:
                target = int(insn.op_str, 0)
            except ValueError:
                return None                              # an indirect jump: through a table or a vtable
            return cleanup(image, target, depth - 1) if depth else None
        if insn.mnemonic == "int3":
            return None                                  # the padding after a body that ends in a jump
        leading = leading and not (insn.mnemonic.startswith(("j", "call", "loop")))
    return None


def heap(symbols: dict[str, int]) -> tuple[Callee, ...]:
    """The game heap on both sides: the original's operator new and free, and the rebuilt build's global operator
    new and delete forms, which src/units/0211 routes to the same functions."""
    return tuple(Callee(original_, symbols[name], 0, 1)
                 for original_, names in GAME_HEAP.items() for name in names if name in symbols)


def stack_words(f: bridge_sources.Function, popped: int) -> tuple[str, ...]:
    """The type of each stack word the entry takes: the hidden result pointer, then the parameters; a class passed
    by value fills the bytes the others leave, shared evenly when there are several."""
    known = sum(s for s in f.sizes if s is not None) + 4 * f.hidden
    unknown = f.sizes.count(None)
    share = (popped - known) // unknown if unknown else 0
    if unknown and (share <= 0 or share % 4 or known + share * unknown != popped):
        raise ValueError(f"{f.address:08X}: cannot place {unknown} classes passed by value in {popped} bytes")
    words = ["void*"] if f.hidden else []
    for type_, size in zip(f.params, f.sizes):
        words += [type_] * (size // 4) if size is not None else ["word"] * (share // 4)
    return tuple(words)


def return_mask(f: bridge_sources.Function) -> int | None:
    if f.result == "void":
        return None
    if f.result != "eax":
        raise ValueError(f"{f.address:08X}: a result in {f.result} is not compared (EAX only)")
    if f.hidden or f.kind == "constructor":
        return 0xFFFFFFFF
    size = 4 if f.returns.endswith(("*", "&")) else bridge_sources.size_of(f.returns, "") or 4
    return (1 << (8 * min(size, 4))) - 1


def targets(symbols: dict[str, int], original_image: Image, only: set[int] | None = None) -> list[Target]:
    """Every bridged function, with its rebuilt entry, forwards, bound globals and vtables."""
    found, runtime = [], heap(symbols)
    for unit in bridge_sources.units():
        plan = unit.plan
        forwards = [(f, Callee(f.function.address, *entry(symbols, f.symbol))) for f in plan.forwards]
        defects = [f"forward {f.symbol} pops {c.popped} bytes, the original {c.original:08X} {expected}"
                   for f, c in forwards if (expected := cleanup(original_image, c.original)) != c.popped
                   and not (expected is None and c.original in unit.bridged)]    # checked as a target itself
        globals_ = tuple((s.address, storage_symbol(symbols, s)) for s in plan.globals)
        for f in unit.functions:
            if only and f.address not in only:
                continue
            table = plan.tables.get(f.address)
            rebuilt, popped = entry(symbols, f.entry)
            found.append(Target(f"{f.owner}::{f.name}", f.address, rebuilt, stack_words(f, popped), popped,
                                return_mask(f), (*(c for _, c in forwards), *runtime), globals_,
                                ((table.address, storage_symbol(symbols, table)),) if table else (),
                                "; ".join(defects) or None))
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
        if type_ == "word":                              # part of a class passed by value
            return self.word(rng)
        if type_.endswith(("*", "&")):
            return rng.choice([self.pointer(rng), rng.choice(self.objects)])
        value = rng.choice([rng.randrange(9), self.outside(1 << rng.randrange(32)), rng.randrange(64), 0xFFFFFFFF,
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

def run(image: Image, start: int, target: Target, inputs: Inputs, rebuilt: bool) -> tuple | str | None:
    """Everything observable after one run, None when it faults, or EXHAUSTED when it used up its steps."""
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(image.base, (len(image.data) + 0xFFF) & ~0xFFF)
    uc.mem_write(image.base, image.data)
    for base, size in ((SANDBOX, SIZE), (STACK, STACK_SIZE), (RETURN, 0x1000), (STUBS, 0x1000), (THREAD, 0x2000)):
        uc.mem_map(base, size)
    thread(uc)
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
    slots: dict[int, str] = {}

    def canonical(uc_: Uc, value: int | None) -> object:
        """A recorded word the two programs can agree on: a stack address as its slot's order of first use (frames
        differ in layout, not in which temporaries are passed where), an address in the image's .rdata as the
        string there (each program has its own copy of a literal), anything else as is."""
        if value is not None and STACK <= value < STACK + STACK_SIZE:
            return slots.setdefault(value, f"stack{len(slots)}")
        if value is not None and image.constant(value):
            text = bytes(uc_.mem_read(value, min(64, image.end - value)))
            return "image:" + text.split(b"\0", 1)[0].decode("latin-1")
        return value

    def stop(uc_: Uc, address: int, _size: int, _data: object) -> None:
        callee = callees.get(address)
        if callee is None and not STUBS <= address < STUBS + 16 * STUB_COUNT:
            return
        sp = uc_.reg_read(UC_X86_REG_ESP)
        popped = callee.popped if callee else 0
        words = callee.recorded if callee and callee.recorded >= 0 else popped // 4
        args = struct.unpack(f"<{words}I", uc_.mem_read(sp + 4, 4 * words)) if words else ()
        this = uc_.reg_read(UC_X86_REG_ECX) if not callee or callee.recorded < 0 else None
        record = (callee.original if callee else address, canonical(uc_, this),
                  tuple(canonical(uc_, a) for a in args))
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
        return EXHAUSTED                                 # still running: neither an end nor a fault
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


def segment(base: int, limit: int, granularity: int) -> int:
    """A present, ring-0, read/write 32-bit data segment descriptor."""
    return ((limit & 0xFFFF) | (base & 0xFFFFFF) << 16 | 0x92 << 40 | (limit >> 16 & 0xF) << 48
            | (0x4 | granularity << 3) << 52 | (base >> 24 & 0xFF) << 56)


def thread(uc: Uc) -> None:
    """A thread's segments: SS, DS and ES flat, and FS a thread block whose exception list (fs:[0]) is empty, as at
    a thread's start. Unicorn's 32-bit mode has no FS base register, so they come from a GDT, which then must give
    every data segment: the stack would otherwise wrap at 64 KiB."""
    gdt = THREAD + 0x1000
    uc.mem_write(gdt, struct.pack("<QQQ", 0, segment(0, 0xFFFFF, 1), segment(THREAD, 0xFFF, 0)))
    uc.reg_write(UC_X86_REG_GDTR, (0, gdt, 3 * 8 - 1, 0))
    for register, selector in ((UC_X86_REG_SS, 0x08), (UC_X86_REG_DS, 0x08), (UC_X86_REG_ES, 0x08),
                               (UC_X86_REG_FS, 0x10)):
        uc.reg_write(register, selector)
    uc.mem_write(THREAD, struct.pack("<I", 0xFFFFFFFF))


def check(target: Target, original_image: Image, rebuilt_image: Image, trials: int) -> Report:
    if target.defect:
        return Report(target, trials, 0, 0, target.defect)
    rng = random.Random(target.original)
    worlds = [World(rng, (original_image, rebuilt_image)) for _ in range(min(WORLDS, trials))]
    agreed = completed = exhausted = 0
    difference = None
    for _ in range(trials):
        world = rng.choice(worlds)
        inputs = Inputs(world, SANDBOX + 4 * rng.randrange(THIS_RANGE // 4),
                        tuple(world.argument(rng, t) for t in target.params),
                        tuple(rng.choice([world.pointer(rng), rng.choice(world.objects)]) for _ in target.globals))
        a = run(original_image, target.original, target, inputs, rebuilt=False)
        b = run(rebuilt_image, target.rebuilt, target, inputs, rebuilt=True)
        if EXHAUSTED in (a, b):
            exhausted += 1                               # the rebuilt code's /Ob0 calls cost more steps per loop
        elif a == b:
            agreed += 1
            completed += a is not None
        elif difference is None:
            difference = describe(a, b, inputs)
    return Report(target, trials, agreed, completed, difference, exhausted)


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
               for t in targets(link_symbols(args.build / "link.map"), original_image, only)]
    for r in reports:
        verdict = "PASS" if r.passed else "FAIL" if r.difference else "INCONCLUSIVE"
        print(f"{verdict:12} {r.target.original:08X} {r.target.name}: {r.agreed}/{r.trials} agree, "
              f"{r.completed} ran to the end" + (f", {r.exhausted} used up their steps" if r.exhausted else "")
              + (f"; {r.difference}" if r.difference else ""))
    failed = [r for r in reports if not r.passed]
    print(f"BRIDGE GATE {'PASS' if not failed else 'FAIL'}: {len(reports) - len(failed)}/{len(reports)} functions")
    return 1 if failed or not reports else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError) as exc:
        print(f"Bridge gate stopped: {exc}", file=sys.stderr)
        sys.exit(1)
