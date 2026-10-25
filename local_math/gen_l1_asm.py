"""gen_l1_asm.py <swkotor.exe> <outdir> : L1 MATH-CRT transcription generator (read-only on the install).

Reads swkotor.exe (GOG 1.03) and writes, in instruction order, the MASM (ml 14.44) transcription of the statically
linked VC7 CRT math routines that L1 owns, plus the data they read:
  <outdir>/kotor_vc7_math.asm        code (one line per original instruction, original address in the comment) and the
                                     small constants, name strings and flags (bit patterns as in the image);
  <outdir>/kotor_vc7_pow_tables.inc  the __pow_pentium4 lookup tables 0075A9C0..0075E2E8 (14,632 bytes of numeric data).
  <outdir>/l1_layout.json            what l1_bytegate.py needs to check the assembled object against the original.
The code set is the recursive-descent closure of the L1 entry points, cut at the boundaries the coordinator ruled on
(errno -> UCRT _errno, the VC7 /GS check -> the toolchain's __security_check_cookie, the OS-SSE probe -> the C++
__try/__except replacement, RaiseException -> the import). Every instruction is translated from capstone's decoding;
l1_bytegate.py proves the assembled object equal to the original bytes, relocation-normalized.
Nothing from the install is stored by this tool except into <outdir>."""
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

import capstone
import pefile
from capstone import x86

if capstone.__version__ != "5.0.7" or pefile.__version__ != "2024.8.26":
    raise SystemExit("Requires capstone 5.0.7 and pefile 2024.8.26")
EXE_SHA256 = "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435"

# ---------------------------------------------------------------- names (original address -> transcription label)
# public: game-visible C names (ruling: standard names), project names for _finite / __CI* / internal state.
ENTRY = {
    0x006FC0E0: ('_kotor_vc7_CIacos', True, '_CIacos: ST0 argument (80-bit), qword copy for classification'),
    0x006FC0F4: ('_acos', True, 'C acos(double): __fload_withFB, then falls into the shared body'),
    0x006FC0FD: ('kv_acos_body', False, 'shared acos body (expects ST0, EAX and flags set up)'),
    0x006FC1B0: ('_pow', True, 'C pow(double, double): SSE2 dispatch on [00834314]'),
    0x006FC1F0: ('_kotor_vc7_CIpow', True, '_CIpow: ST1 = x, ST0 = y; SSE2 dispatch on [00834314]'),
    0x006FC24D: ('kv_pow_internal', False, '_pow_internal (x87)'),
    0x006FC412: ('kv_test_whether_TOS_is_int', False, '_test_whether_TOS_is_int'),
    0x006FC750: ('_floor', True, 'C floor(double): SSE2 path inline, x87 path 00705672'),
    0x006FC880: ('_ceil', True, 'C ceil(double): SSE2 path inline, x87 path 00705743'),
    0x006FCFF0: ('_kotor_vc7_CIasin', True, '_CIasin: ST0 argument (80-bit)'),
    0x006FD004: ('_asin', True, 'C asin(double)'),
    0x006FD00D: ('kv_asin_body', False, 'shared asin body'),
    0x006FE517: ('_kotor_vc7_finite', True, '_finite(double) (project name, ruling 2)'),
    0x006FE52C: ('kv_fpclass', False, '_fpclass'),
    0x007041A0: ('kv_startTwoArgErrorHandling', False, '_startTwoArgErrorHandling'),
    0x007041B7: ('kv_startOneArgErrorHandling', False, '_startOneArgErrorHandling'),
    0x00704200: ('kv_twoToTOS', False, '_twoToTOS: 2^ST0'),
    0x00704215: ('kv_load_CW', False, '_load_CW: CW = (caller CW & 0300h) | 007Fh: keeps the precision control, RC nearest, all exceptions masked'),
    0x0070422C: ('kv_convertTOStoQNaN', False, '_convertTOStoQNaN'),
    0x00704245: ('kv_fload_withFB', False, '_fload_withFB: fld qword [edx] (NaN/inf through a tbyte)'),
    0x00704288: ('kv_checkTOS_withFB', False, '_checkTOS_withFB'),
    0x0070429E: ('kv_fast_exit', False, '_fast_exit: restore CW, pop, ret'),
    0x007042AB: ('kv_math_exit', False, '_math_exit: precision-exception check'),
    0x007042E9: ('kv_check_range_exit', False, '_check_range_exit'),
    0x007043C1: ('_kotor_vc7_sse2_available_init', True, '__sse2_available_init (registered in .CRT$XIC)'),
    0x00704420: ('kv_CIpow_pentium4', False, '_CIpow SSE2 entry'),
    0x00704439: ('kv_pow_pentium4', False, '__pow_pentium4'),
    0x00705177: ('kv_pow_y_is_int', False, 'pow helper: classify y (0 non-integer, 1 odd, 2 even)'),
    0x007051E1: ('kv_powhlp', False, '_powhlp'),
    0x007053DE: ('kv_sse2_math_error', False, 'SSE2 path error handler (VC7 libm error support)'),
    0x00705672: ('kv_floor_x87', False, 'floor x87 path'),
    0x00705743: ('kv_ceil_x87', False, 'ceil x87 path'),
    0x00706A22: ('kv_matherr', False, 'default _matherr: returns 0 (ruling: reconstruction of the original default)'),
    0x00707600: ('kv_set_exp', False, '_set_exp'),
    0x0070762A: ('kv_sptype', False, '_sptype'),
    0x00707685: ('kv_decomp', False, '_decomp'),
    0x0070773F: ('kv_raise_exc', False, '_raise_exc (RaiseException for unmasked exceptions)'),
    0x007079E4: ('kv_handle_exc', False, '_handle_exc'),
    0x00707C06: ('kv_set_errno_from_matherr', False, '_set_errno_from_matherr'),
    0x00707C2E: ('kv_errcode', False, '_errcode'),
    0x00707C5B: ('kv_umatherr', False, '_umatherr'),
    0x00707CF9: ('kv_handle_qnan1', False, '_handle_qnan1'),
    0x00707D4C: ('kv_except1', False, '_except1'),
    0x00707E0A: ('kv_statfp', False, '_statfp'),
    0x00707E17: ('kv_clrfp', False, '_clrfp'),
    0x00707E25: ('kv_ctrlfp', False, '_ctrlfp'),
    0x00707E49: ('kv_set_statfp', False, '_set_statfp'),
    0x007096B2: ('kv_87except', False, '_87except'),
    0x0070A53B: ('kv_frnd', False, '_frnd'),
}
ROOTS = [0x006FC0E0, 0x006FC0F4, 0x006FCFF0, 0x006FD004, 0x006FC1B0, 0x006FC1F0, 0x006FC750, 0x006FC880, 0x006FE517, 0x007043C1]
# Boundary rebinding (coordinator rulings): call target -> external symbol.
STOP_CALL = {
    0x006FFE78: '__errno',                           # VC7 per-thread errno -> UCRT _errno() (no VC7 TLS)
    0x00705D37: '@__security_check_cookie@4',     # VC7 /GS check -> toolchain check (fastcall ECX)
    0x0070438C: '_kotor_vc7_has_osfxsr_set',       # OS SSE probe (VC7 SEH frame) -> C++ __try/__except, same result
}
EXT_MEM = {0x0073D148: '__imp__RaiseException@16', 0x007A3180: '___security_cookie'}

# ---------------------------------------------------------------- data blocks (start, end, segment, comment)
BLOCKS = [
    (0x0073EDE8, 0x0073EDF0, 'R', 'double 1.0'),
    (0x0073EDF8, 0x0073EE00, 'R', 'double 0.0'),
    (0x00741DF8, 0x00741E00, 'R', 'double 2.0'),
    (0x0074DC58, 0x0074DC5C, 'R', '"log"'),
    (0x0075A0B0, 0x0075A148, 'R', 'floor/ceil SSE2 constants (16-byte aligned operands)'),
    (0x0075A960, 0x0075A9A4, 'R', 'x87 range-check constants'),
    (0x0075A9C0, 0x0075E2E8, 'T', '__pow_pentium4 tables'),
    (0x0075E358, 0x0075E3A0, 'R', 'SSE2 error-handler function names'),
    (0x0075F764, 0x0075F7F8, 'R', '_umatherr opcode-table names'),
    (0x007A2770, 0x007A2790, 'D', '"acos", double 0.5, "pow"'),
    (0x007A27B0, 0x007A27B8, 'D', '"asin"'),
    (0x007A30E0, 0x007A30F4, 'D', 'tbyte real indefinite, tbyte pi/2'),
    (0x007A3110, 0x007A311A, 'D', 'tbyte +inf'),
    (0x007A316C, 0x007A3178, 'D', '_matherr pointer, floor CW 173F, ceil CW 1B3F'),
    (0x007A32B0, 0x007A33D6, 'D', 'doubles, _umatherr opcode/name table, _set_statfp tbytes'),
    (0x007A36D0, 0x007A36D4, 'D', '_matherr_flag = 9876 (link-time constant; no writer in the image)'),
    (0x00833BDC, 0x00833BE0, 'B', 'flag 00833BDC: constant 0 (no writer; reads kept, PLAN-v3 2.1)'),
    (0x00834314, 0x00834318, 'B', '__use_sse2_mathfcns'),
    (0x00834318, 0x0083431C, 'B', '__sse2_available'),
]
DATA_NAME = {0x00834314: ('_kotor_vc7_use_sse2_mathfcns', True), 0x00834318: ('_kotor_vc7_sse2_available', True),
             0x00833BDC: ('kv_flag_833BDC', False), 0x007A36D0: ('kv_matherr_flag', False), 0x007A316C: ('kv_pmatherr', False)}
# dwords inside data blocks that are pointers in the image (relocated as OFFSET <label>)
PTR_SLOTS = [0x007A316C] + [0x007A32DC + 8 * i for i in range(29)]


DATA_START, DATA_END = 0x0073D000, 0x00840000     # .rdata .. end of .data: immediates here are addresses
FILE_END = 0x007A4000                              # data at and past this address is loader-zero
BRANCHES = ('loop', 'jecxz')
X87_ARITHMETIC = ('fadd', 'fsub', 'fsubr', 'fmul', 'fdiv', 'fdivr', 'faddp', 'fsubp', 'fsubrp', 'fmulp', 'fdivp',
                  'fdivrp')
HEADER = (
    '; kotor_vc7_math.asm - L1 MATH-CRT: transcription of the VC7 CRT math routines statically linked into',
    ';   swkotor.exe (GOG 1.03): acos / asin (C entries and _CI entries, shared bodies), pow (C, _CI, x87 _pow_internal,',
    ';   SSE2 __pow_pentium4), floor / ceil (SSE2 and x87 paths), _finite, __sse2_available_init, and the x87 error',
    ';   machinery they reach (_startOneArgErrorHandling .. _87except, _except1, _handle_exc, _raise_exc).',
    '; Generated by tools/gen_l1_asm.py from the install, one line per original instruction in original order;',
    ';   the comment is the original address (no original bytes are written into this file except the constants and the',
    ';   9 instructions ML cannot encode with their zero disp8). tools/l1_bytegate.py proves the assembled bytes equal',
    ';   to the original, relocation-normalized, reading the install at run time.',
    ';   Rebound boundaries (coordinator rulings, PLAN-v2 8.4 / PLAN-v3 2.1):',
    ';   006FFE78 _errno -> UCRT _errno (__errno); 00705D37 __security_check_cookie and [007A3180] -> toolchain /GS;',
    ';   0070438C OS-SSE probe -> _kotor_vc7_has_osfxsr_set (C++ __try/__except, same result);',
    ';   [0073D148] RaiseException -> __imp__RaiseException@16. Flags 00833BDC (0) and 007A36D0 (9876) keep their reads.',
    '; Not exported here: __ftol2 (006FAE8C, Task25 provider). __CI* and _finite carry project names (ruling 2):',
    ';   C symbols _kotor_vc7_CIacos/_CIasin/_CIpow (register ABI), _kotor_vc7_finite, _kotor_vc7_sse2_available_init,',
    ';   data _kotor_vc7_use_sse2_mathfcns / _kotor_vc7_sse2_available (C names without the leading underscore).',
    '.686P',
    '.XMM',
    '.MODEL FLAT',
    'OPTION CASEMAP:NONE',
    '',
)


class Original:
    """The hash-pinned swkotor.exe as the loader maps it, with a 32-bit decoder."""

    def __init__(self, path):
        if hashlib.sha256(Path(path).read_bytes()).hexdigest() != EXE_SHA256:
            raise SystemExit("Original executable hash mismatch")
        pe = pefile.PE(str(path), fast_load=True)
        self.base = pe.OPTIONAL_HEADER.ImageBase
        self.image = pe.get_memory_mapped_image()
        self.decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.decoder.detail = True

    def u32(self, a):
        return struct.unpack_from('<I', self.image, a - self.base)[0]

    def bytes(self, start, end):
        """Image bytes start..end; zeros for the loader-zero tail."""
        return self.image[start - self.base:end - self.base] if start < FILE_END else bytes(end - start)

    def instruction(self, va):
        return next(self.decoder.disasm(self.image[va - self.base:va - self.base + 16], va))


def is_branch(mnemonic):
    return mnemonic.startswith('j') or mnemonic in BRANCHES


def closure(original):
    """{address: instruction} reachable from ROOTS, not following rebound calls."""
    seen = {}
    work = list(ROOTS)
    while work:
        va = work.pop()
        while va not in seen:
            ins = original.instruction(va)
            seen[va] = ins
            m = ins.mnemonic
            if m == 'call':
                op = ins.operands[0]
                if op.type == x86.X86_OP_IMM and op.imm not in STOP_CALL:
                    work.append(op.imm)
            elif m == 'jmp':
                op = ins.operands[0]
                if op.type == x86.X86_OP_IMM:
                    work.append(op.imm)
                break
            elif is_branch(m):
                work.append(ins.operands[0].imm)
            if m in ('ret', 'int3', 'hlt'):
                break
            va += ins.size
    return seen


def hexs(v):
    return f'0{v:X}h'


def block_of(a):
    return next((s for s, e, _, _ in BLOCKS if s <= a < e), None)


def data_label(a):
    return DATA_NAME[a][0] if a in DATA_NAME else f'D_{a:08X}'


def runs_of(seen):
    """Address-ordered runs of contiguous instructions; each must end in ret or jmp."""
    runs = []
    for a in sorted(seen):
        if runs and runs[-1][-1] + seen[runs[-1][-1]].size == a:
            runs[-1].append(a)
        else:
            runs.append([a])
    for r in runs:
        last = seen[r[-1]]
        if last.mnemonic not in ('ret', 'jmp'):
            raise SystemExit(f'run {r[0]:08X}..{r[-1]:08X} ends in {last.mnemonic} (fall-through would be lost)')
    return runs


class Transcription:
    """MASM text for the closure and the data it references."""

    def __init__(self, original, seen):
        self.original = original
        self.seen = seen
        self.targets = set(ENTRY)
        self.refs = set()           # data addresses referenced
        for ins in seen.values():
            if ins.mnemonic.startswith('j') or ins.mnemonic == 'call':
                op = ins.operands[0]
                if op.type == x86.X86_OP_IMM:
                    self.targets.add(op.imm)
            for op in ins.operands:
                if op.type == x86.X86_OP_MEM and op.mem.disp >= 0x400000 and op.mem.disp not in EXT_MEM:
                    self.refs.add(op.mem.disp)
                if (op.type == x86.X86_OP_IMM and DATA_START <= op.imm < DATA_END and ins.mnemonic != 'call'
                        and not ins.mnemonic.startswith('j')):
                    self.refs.add(op.imm)
        for a in PTR_SLOTS:
            if original.u32(a) not in ENTRY:
                self.refs.add(original.u32(a))
        for r in sorted(self.refs):
            if block_of(r) is None:
                raise SystemExit(f'data reference {r:08X} outside every block')

    def label(self, target):
        if target in STOP_CALL:
            return STOP_CALL[target]
        if target in ENTRY:
            return ENTRY[target][0]
        if target not in self.seen:
            raise SystemExit(f'branch target {target:08X} not transcribed')
        return f'L{target:08X}'

    @staticmethod
    def symbol(v):
        return EXT_MEM[v] if v in EXT_MEM else data_label(v)

    def translate(self, ins):
        """(mnemonic, operands) for one instruction, encoded as the original is."""
        m, ops = ins.mnemonic, ins.op_str
        if m == 'wait':
            return 'fwait', ''
        # [esp+0] written with an explicit zero disp8 (the VC7 CRT's own inline asm): MASM always drops a zero
        # displacement, so these are emitted as their exact bytes; the comment carries the instruction.
        if any(op.type == x86.X86_OP_MEM and op.mem.disp == 0 for op in ins.operands) and ins.disp_size == 1:
            return 'db', f"{', '.join(hexs(x) for x in ins.bytes)} ; {m} {ops} (zero disp8 kept)"
        if m == 'call' or is_branch(m):
            op = ins.operands[0]
            if op.type == x86.X86_OP_IMM:
                if m == 'call':
                    return 'call', self.label(op.imm)
                return m, ('SHORT ' if ins.size == 2 else 'NEAR PTR ') + self.label(op.imm)
        # x87 register forms: spell both operands the way MASM encodes them (D8: st, st(i); DC/DE: st(i), st)
        b0 = ins.bytes[0]
        if m in X87_ARITHMETIC and ins.bytes[1] >= 0xC0 and b0 in (0xD8, 0xDC, 0xDE):
            regs = re.findall(r'st\((\d)\)', ops)
            i = [r for r in regs if r != '0'] or ['0']
            return m, f'st, st({i[0]})' if b0 == 0xD8 else f'st({i[0]}), st'
        # TEST r/m8|r/m32, reg: MASM puts the FIRST register operand in ModRM.reg, the original has it in ModRM.rm;
        # TEST is commutative, so the operands are written in reverse to keep the original encoding (e.g. 84 CD).
        if m == 'test' and len(ins.operands) == 2 and all(op.type == x86.X86_OP_REG for op in ins.operands):
            a, b = (x.strip() for x in ops.split(','))
            return m, f'{b}, {a}'
        if m in ('fnsave', 'frstor', 'fnstenv', 'fldenv'):
            ops = re.sub(r'^\w+ ptr ', '', ops)
        ops = ops.replace('xword ptr', 'tbyte ptr')
        # memory displacements and address immediates -> symbols
        mem_disp = {op.mem.disp for op in ins.operands if op.type == x86.X86_OP_MEM}

        def symbolize(mo):
            v = int(mo.group(0), 16)
            if v >= 0x400000 and (v in mem_disp or v in EXT_MEM):
                return self.symbol(v)
            if DATA_START <= v < DATA_END:
                return 'OFFSET ' + self.symbol(v)
            return hexs(v)
        return m, re.sub(r'0x[0-9a-fA-F]+', symbolize, ops)

    def data_block(self, w, s, e, comment, ptrs):
        original = self.original
        w(f'; ---- {s:08X}..{e:08X} {comment}')
        a = s
        while a < e:
            if a in self.refs or a == s:
                w(f'{data_label(a)} LABEL BYTE')
            if a in ptrs:
                target = original.u32(a)
                name = self.label(target) if target in ENTRY else data_label(target)
                w(f'    dd OFFSET {name} ; {a:08X} -> {target:08X}')
                a += 4
                continue
            nxt = min([x for x in [*self.refs, *ptrs, e] if a < x <= e] + [a + 16])
            nxt = min(nxt, (a // 16 + 1) * 16, e)
            w(f"    db {', '.join(hexs(x) for x in original.bytes(a, nxt))} ; {a:08X}")
            a = nxt

    def pow_tables(self):
        lines = []
        for s, e, seg, _ in BLOCKS:
            if seg != 'T':
                continue
            lines.append(f'; kotor_vc7_pow_tables.inc - __pow_pentium4 lookup tables {s:08X}..{e:08X} (numeric data '
                         'from the image,')
            lines.append(';   generated by tools/gen_l1_asm.py; relative layout and 64-byte alignment as in the '
                         'original).')
            a = s
            while a < e:
                if a in self.refs:
                    lines.append(f'{data_label(a)} LABEL BYTE')
                nxt = min([x for x in self.refs if a < x < e] + [(a // 16 + 1) * 16, e])
                data = self.original.image[a - self.original.base:nxt - self.original.base]
                lines.append(f"    db {', '.join(hexs(x) for x in data)} ; {a:08X}")
                a = nxt
        return lines

    def assembly(self, runs):
        """(MASM lines, transcribed instruction addresses in emission order)."""
        lines = list(HEADER)
        w = lines.append
        for _, (name, public, _) in sorted(ENTRY.items()):
            if public:
                w(f'PUBLIC {name}')
        for _, (name, public) in sorted(DATA_NAME.items()):
            if public:
                w(f'PUBLIC {name}')
        for name in sorted(set(STOP_CALL.values())):
            w(f'EXTERN {name}:NEAR')
        w('EXTERN __imp__RaiseException@16:DWORD')
        w('EXTERN ___security_cookie:DWORD')
        w('')
        w('KV_RDATA SEGMENT READONLY ALIGN(64) PUBLIC FLAT ALIAS(".rdata") \'CONST\'')
        for s, e, seg, c in BLOCKS:
            if seg == 'R':
                w('    ALIGN 16')
                self.data_block(w, s, e, c, [])
            if seg == 'T':
                w('    ALIGN 64')
                w('    INCLUDE kotor_vc7_pow_tables.inc')
        w('KV_RDATA ENDS')
        w('')
        w('KV_DATA SEGMENT ALIGN(16) PUBLIC FLAT ALIAS(".data") \'DATA\'')
        for s, e, seg, c in BLOCKS:
            if seg in 'DB':
                w('    ALIGN 4')
                self.data_block(w, s, e, c, [p for p in PTR_SLOTS if s <= p < e])
        w('KV_DATA ENDS')
        w('')
        w('.CODE')
        order = []
        for r in runs:
            w('')
            w(f'; ======== {r[0]:08X}..{r[-1] + self.seen[r[-1]].size:08X}')
            for a in r:
                if a in ENTRY:
                    w('')
                    w(f'; {a:08X} {ENTRY[a][2]}')
                    w(f'{ENTRY[a][0]}:')
                elif a in self.targets:
                    w(f'L{a:08X}:')
                m, ops = self.translate(self.seen[a])
                w(f'    {m:<9} {ops:<44} ; {a:08X}')
                order.append(a)
        w('')
        w('END')
        return lines, order


def layout(order, runs, seen):
    return {'instructions': order, 'runs': [[r[0], r[-1] + seen[r[-1]].size] for r in runs],
            'entries': {f'{a:08X}': v[0] for a, v in ENTRY.items()},
            'stops': {f'{a:08X}': v for a, v in STOP_CALL.items()},
            'ext_mem': {f'{a:08X}': v for a, v in EXT_MEM.items()},
            'blocks': [[f'{s:08X}', f'{e:08X}', seg] for s, e, seg, _ in BLOCKS],
            'data_names': {f'{a:08X}': v[0] for a, v in DATA_NAME.items()},
            'ptr_slots': [f'{p:08X}' for p in PTR_SLOTS]}


def write_crlf(path, lines):
    path.write_bytes(('\n'.join(lines) + '\n').replace('\n', '\r\n').encode())


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.splitlines()[0])
    exe, out = Path(sys.argv[1]), Path(sys.argv[2]).resolve()
    source, installation = Path(__file__).resolve().parents[1], exe.resolve().parent
    if any(out.is_relative_to(p) or p.is_relative_to(out) for p in (source, installation)):
        raise SystemExit("Generated output must be separate from source and installation")
    original = Original(exe)
    out.mkdir(parents=True, exist_ok=False)
    (out / '.gitignore').write_bytes(b'*\r\n')
    seen = closure(original)
    runs = runs_of(seen)
    transcription = Transcription(original, seen)
    lines, order = transcription.assembly(runs)
    write_crlf(out / 'kotor_vc7_pow_tables.inc', transcription.pow_tables())
    write_crlf(out / 'kotor_vc7_math.asm', lines)
    (out / 'l1_layout.json').write_bytes(json.dumps(layout(order, runs, seen), indent=1).replace('\n', '\r\n').encode())
    print(f'instructions {len(order)} in {len(runs)} runs; data refs {len(transcription.refs)}; wrote {out}')


if __name__ == '__main__':
    main()
