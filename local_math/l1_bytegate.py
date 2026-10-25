"""l1_bytegate.py <swkotor.exe> <obj> <lst> <l1_layout.json> [--mutant] : relocation-normalized byte gate for the
L1 transcription.

Proves the assembled kotor_vc7_math.obj equal to the original swkotor.exe bytes (read-only from the install):
  code  every transcribed instruction, in emission order, has the original length and bytes; the only differences
        allowed are (a) internal branch displacements, which must reach the transcription of the ORIGINAL target,
        (b) relocated fields, whose relocation must name the transcription of the ORIGINAL address (data label
        D_<addr> / named flag, code label, or the ruled external: _errno, @__security_check_cookie@4,
        kotor_vc7_has_osfxsr_set, __imp__RaiseException@16, ___security_cookie);
  data  every data block equals the image bytes (.bss blocks: zero), pointer slots relocate to the ORIGINAL target;
  size  the .text section holds exactly the transcribed instructions and nothing else.
Exit 0 = PASS. A mutant object (--mutant) is expected to FAIL; the exit code is inverted in that mode."""
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

import capstone
import pefile

EXE_SHA256 = "9c10e0450a6eeca417e036e3cde7474fed1f0a92aab018446d156944dea91435"
REL32, DIR32 = 0x14, 0x06
STATIC = 3
FILE_END = 0x007A4000                       # data at and past this address is loader-zero
SEGMENT_SECTIONS = {'_TEXT': '.text$mn', 'KV_RDATA': '.rdata', 'KV_DATA': '.data'}
LISTING_SYMBOL = re.compile(r'^(\S+)\s+[. ]*\.\s+(?:Byte|L Near|DWord|Number|Text)\s+([0-9A-F]{8})\s+(\w+)')


def coff(path):
    """([{name, data, rels, chars}], {index: {name, value, sec, cls}}) of an i386 COFF object."""
    b = Path(path).read_bytes()
    machine, section_count, _, symbol_pointer, symbol_count, optional, _ = struct.unpack_from('<HHIIIHH', b, 0)
    assert machine == 0x14C, 'not an i386 COFF object'
    strings = symbol_pointer + symbol_count * 18

    def string(offset):
        return b[strings + offset:b.index(b'\0', strings + offset)].decode()

    sections = []
    for i in range(section_count):
        o = 20 + optional + i * 40
        name = b[o:o + 8].rstrip(b'\0').decode()
        if name.startswith('/'):
            name = string(int(name[1:]))
        _, _, size, pointer, relocation_pointer, _, relocation_count, _, characteristics = struct.unpack_from(
            '<IIIIIIHHI', b, o + 8)
        relocations = [struct.unpack_from('<IIH', b, relocation_pointer + k * 10) for k in range(relocation_count)]
        sections.append({'name': name, 'data': b[pointer:pointer + size] if pointer else b'\0' * size,
                         'rels': relocations, 'chars': characteristics})
    symbols = {}
    i = 0
    while i < symbol_count:
        o = symbol_pointer + i * 18
        raw = b[o:o + 8]
        name = string(struct.unpack_from('<I', raw, 4)[0]) if raw[:4] == b'\0\0\0\0' else raw.rstrip(b'\0').decode()
        value, section, _, storage, aux = struct.unpack_from('<IhHBB', b, o + 8)
        symbols[i] = {'name': name, 'value': value, 'sec': section, 'cls': storage}
        i += 1 + aux
    return sections, symbols


def lst_symbols(path):
    """{label: (segment, offset)} from the MASM listing's symbol table."""
    found = {}
    with open(path, errors='replace') as listing:
        for line in listing:
            m = LISTING_SYMBOL.match(line.rstrip())
            if m:
                found[m.group(1)] = (m.group(3), int(m.group(2), 16))
    return found


class Gate:
    def __init__(self, exe, obj, lst, layout):
        pe = pefile.PE(exe, fast_load=True)
        self.base = pe.OPTIONAL_HEADER.ImageBase
        self.image = pe.get_memory_mapped_image()
        self.decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self.decoder.detail = True
        self.layout = layout
        self.sections, self.symbols = coff(obj)
        # MASM also emits an empty default .data; keep the non-empty section of each name.
        self.by_name = {}
        for i, s in enumerate(self.sections):
            if s['name'] not in self.by_name or len(s['data']) > len(self.by_name[s['name']][1]['data']):
                self.by_name[s['name']] = (i + 1, s)
        self.entries = {v: int(k, 16) for k, v in layout['entries'].items()}
        self.data_names = {v: int(k, 16) for k, v in layout['data_names'].items()}
        self.external = {v: int(k, 16) for k, v in layout['ext_mem'].items()}
        self.stops = {int(k, 16): v for k, v in layout['stops'].items()}
        # (section name, offset) -> original address, from the listing
        self.location = {}
        for name, (segment, offset) in lst_symbols(lst).items():
            address = self.label_address(name)
            if address is not None and segment in SEGMENT_SECTIONS:
                self.location[(SEGMENT_SECTIONS[segment], offset)] = address
        self.original_at = {}           # .text$mn offset -> original address
        self.failures = []

    def label_address(self, name):
        if re.fullmatch(r'D_[0-9A-F]{8}', name):
            return int(name[2:], 16)
        if re.fullmatch(r'L[0-9A-F]{8}', name):
            return int(name[1:], 16)
        return self.entries.get(name, self.data_names.get(name))

    def instruction(self, data, address):
        return next(self.decoder.disasm(data, address), None)

    def original(self, address):
        return self.instruction(self.image[address - self.base:address - self.base + 16], address)

    def resolve(self, symbol, addend):
        """('orig', address) for a symbol defined in this object (+ addend), ('ext', name) otherwise."""
        if symbol['sec'] > 0:           # a section symbol or local label (static), or a public defined here
            name, offset = self.sections[symbol['sec'] - 1]['name'], symbol['value'] + addend
            if name == '.text$mn':
                return 'orig', self.original_at.get(offset)
            return 'orig', self.location.get((name, offset))
        return 'ext', symbol['name']

    def check_code(self):
        fail = self.failures.append
        text = self.by_name['.text$mn'][1]
        code = text['data']
        relocations = {offset: (self.symbols[index], kind) for offset, index, kind in text['rels']}
        offsets, offset = {}, 0
        for address in self.layout['instructions']:
            offsets[address] = offset
            offset += self.original(address).size
        if offset != len(code):
            fail(f'size: .text is {len(code)} bytes, transcription {offset}')
        self.original_at = {o: a for a, o in offsets.items()}
        counts = {'instructions': 0, 'relocations': 0, 'branches': 0}
        for address in self.layout['instructions']:
            o = offsets[address]
            theirs = self.original(address)
            n = theirs.size
            mine, orig = code[o:o + n], bytes(theirs.bytes)
            ours = self.instruction(mine, o)
            if ours is None or ours.size != n:
                fail(f'{address:08X}: length/decoding differs ({mine.hex()})')
                continue
            if any(r in relocations and r + 4 > o + n for r in range(o, o + n)):
                fail(f'{address:08X}: relocation straddles the instruction end (instruction lengths drifted)')
                break
            mask = bytearray(n)
            for r in range(o, o + n):
                if r not in relocations:
                    continue
                symbol, kind = relocations[r]
                counts['relocations'] += 1
                mask[r - o:r - o + 4] = b'\1' * 4
                field = struct.unpack_from('<I', mine, r - o)[0]
                original_field = struct.unpack_from('<I', orig, r - o)[0]
                if kind == REL32:       # call/jmp to an external or another section
                    where, value = self.resolve(symbol, 0)
                    target = (address + n + original_field) & 0xFFFFFFFF
                    if not ((where == 'ext' and self.stops.get(target) == value)
                            or (where == 'orig' and value == target)):
                        fail(f'{address:08X}: REL32 to {where} {value}, original target {target:08X}')
                elif kind == DIR32:
                    where, value = self.resolve(symbol, field)
                    if where == 'ext':
                        if self.external.get(value) != original_field:
                            fail(f'{address:08X}: DIR32 external {value}, original operand {original_field:08X}')
                    elif value != original_field:
                        shown = f'{value:08X}' if value is not None else 'unmapped'
                        fail(f'{address:08X}: DIR32 -> {shown}, original operand {original_field:08X}')
                else:
                    fail(f'{address:08X}: unexpected relocation type {kind:#x}')
            branch = ((theirs.mnemonic.startswith('j') or theirs.mnemonic == 'call') and theirs.operands
                      and theirs.operands[0].type == capstone.x86.X86_OP_IMM)
            if branch and not any(mask):
                counts['branches'] += 1
                width = 1 if n == 2 else 4
                if mine[:n - width] != orig[:n - width]:
                    fail(f'{address:08X}: branch opcode differs {mine.hex()} vs {orig.hex()}')
                reached = self.original_at.get(ours.operands[0].imm)
                if reached != theirs.operands[0].imm:
                    fail(f'{address:08X}: branch reaches {reached}, original target {theirs.operands[0].imm:08X}')
            else:
                for k in range(n):
                    if not mask[k] and mine[k] != orig[k]:
                        fail(f'{address:08X}: byte {k} differs: {mine.hex()} vs original {orig.hex()}')
                        break
            counts['instructions'] += 1
        return counts, len(code)

    def check_data(self):
        fail = self.failures.append
        checked = 0
        for start, end, segment in self.layout['blocks']:
            s, e = int(start, 16), int(end, 16)
            name = '.data' if segment in 'DB' else '.rdata'
            hits = [o for (section, o), v in self.location.items() if section == name and v == s]
            if not hits:
                fail(f'data block {s:08X} not found in {name}')
                continue
            o = hits[0]
            section = self.by_name[name][1]
            data = section['data']
            relocations = {offset: (self.symbols[index], kind) for offset, index, kind in section['rels']}
            want = self.image[s - self.base:e - self.base] if s < FILE_END else b'\0' * (e - s)
            k = 0
            while k < e - s:
                if (o + k) in relocations:
                    symbol, _ = relocations[o + k]
                    _, value = self.resolve(symbol, struct.unpack_from('<I', data, o + k)[0])
                    expected = struct.unpack_from('<I', want, k)[0]
                    if value != expected:
                        fail(f'data {s + k:08X}: pointer -> {value}, original {expected:08X}')
                    k += 4
                    continue
                if data[o + k] != want[k]:
                    fail(f'data {s + k:08X} differs: {data[o + k]:02X} vs {want[k]:02X}')
                    break
                k += 1
                checked += 1
        return checked


def main():
    arguments = [a for a in sys.argv[1:] if a != '--mutant']
    mutant = '--mutant' in sys.argv[1:]
    if len(arguments) != 4:
        raise SystemExit(__doc__.splitlines()[0])
    exe, obj, lst, layout = arguments
    if hashlib.sha256(Path(exe).read_bytes()).hexdigest() != EXE_SHA256:
        raise SystemExit("Original executable hash mismatch")
    gate = Gate(exe, obj, lst, json.loads(Path(layout).read_text()))
    counts, text_size = gate.check_code()
    data_bytes = gate.check_data()
    for failure in gate.failures[:40]:
        print('FAIL', failure)
    ok = not gate.failures
    print(f"L1 BYTEGATE {'PASS' if ok else 'FAIL'}: {counts['instructions']} instructions ({text_size} bytes), "
          f"{counts['relocations']} relocated fields, {counts['branches']} internal branches, {data_bytes} data bytes; "
          f"{len(gate.failures)} failures")
    if mutant:
        print(f"MUTANT {'CAUGHT' if not ok else 'MISSED'}")
        sys.exit(0 if not ok else 1)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
