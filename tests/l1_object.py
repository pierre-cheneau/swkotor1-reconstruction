"""The object and listing ml.exe would make from the L1 fixture's transcription.

l1_bytegate.py checks MASM's output against the original. Without ml.exe,
this assembles the same thing from the layout gen_l1_asm.py wrote: code in
emission order with a relocation on every field that names a symbol, the
data blocks in .rdata/.data, and the listing's label table.
"""
from __future__ import annotations

import json
import struct
from pathlib import Path

import capstone
import pefile
from capstone import x86

import synthetic_coff as sc

DATA_START, DATA_END, FILE_END = 0x0073D000, 0x00840000, 0x007A4000


def build(exe: Path, layout_path: Path, out: Path) -> tuple[Path, Path]:
    layout = json.loads(layout_path.read_text())
    pe = pefile.PE(str(exe), fast_load=True)
    base, image = pe.OPTIONAL_HEADER.ImageBase, pe.get_memory_mapped_image()
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    entries = {int(k, 16): v for k, v in layout["entries"].items()}
    stops = {int(k, 16): v for k, v in layout["stops"].items()}
    external = {int(k, 16): v for k, v in layout["ext_mem"].items()}
    names = {int(k, 16): v for k, v in layout["data_names"].items()}

    # Data sections: blocks in order, each label at its offset.
    sections = {".rdata": bytearray(), ".data": bytearray()}
    where: dict[int, tuple[str, int]] = {}
    for s, e, segment in layout["blocks"]:
        s, e = int(s, 16), int(e, 16)
        section = ".data" if segment in "DB" else ".rdata"
        where[s] = (section, len(sections[section]))
        sections[section] += image[s - base:e - base] if s < FILE_END else bytes(e - s)

    labelled: set[int] = set(where)

    def data_location(address: int) -> tuple[str, int]:
        """Section and offset of a data address; MASM labels every referenced one."""
        labelled.add(address)
        start = max(s for s in where if s <= address)
        section, offset = where[start]
        return section, offset + address - start

    symbols = [sc.SymbolSpec(".text$mn", 0, 1, 0, 3), sc.SymbolSpec(".rdata", 0, 2, 0, 3),
               sc.SymbolSpec(".data", 0, 3, 0, 3)]
    index = {s.name: i for i, s in enumerate(symbols)}

    def external_symbol(name: str) -> int:
        if name not in index:
            index[name] = len(symbols)
            symbols.append(sc.SymbolSpec(name, 0, 0, 0x20, 2))
        return index[name]

    code, text_relocations, offsets, branches = bytearray(), [], {}, []
    for address in layout["instructions"]:
        ins = next(decoder.disasm(image[address - base:address - base + 16], address))
        offsets[address] = len(code)
        field = bytearray(ins.bytes)
        if ins.mnemonic == "call" and ins.operands[0].type == x86.X86_OP_IMM and ins.operands[0].imm in stops:
            text_relocations.append((len(code) + 1, external_symbol(stops[ins.operands[0].imm]), sc.REL32))
        for op in ins.operands:
            value, at = None, None
            if op.type == x86.X86_OP_MEM and op.mem.disp >= 0x400000:
                value, at = op.mem.disp & 0xFFFFFFFF, ins.disp_offset
            elif (op.type == x86.X86_OP_IMM and DATA_START <= op.imm < DATA_END
                  and not (ins.mnemonic == "call" or ins.mnemonic.startswith("j"))):
                value, at = op.imm, ins.imm_offset
            if value is None:
                continue
            if value in external:
                text_relocations.append((len(code) + at, external_symbol(external[value]), sc.DIR32))
            else:
                section, offset = data_location(value)
                struct.pack_into("<I", field, at, offset)
                text_relocations.append((len(code) + at, index[section], sc.DIR32))
        code += field
        if ((ins.mnemonic.startswith("j") or ins.mnemonic == "call") and ins.operands[0].type == x86.X86_OP_IMM
                and ins.operands[0].imm not in stops):
            branches.append((offsets[address], ins.size, ins.operands[0].imm))
    for offset, size, target in branches:     # MASM encodes branches within the transcription
        width = 1 if size == 2 else 4
        displacement = offsets[target] - (offset + size)
        code[offset + size - width:offset + size] = displacement.to_bytes(width, "little", signed=True)
    data_relocations = {".rdata": [], ".data": []}
    for slot in (int(p, 16) for p in layout["ptr_slots"]):
        section, offset = data_location(slot)
        target = struct.unpack_from("<I", image, slot - base)[0]
        if target in entries:
            symbols.append(sc.SymbolSpec(entries[target], offsets[target], 1, 0x20, 2))
            struct.pack_into("<I", sections[section], offset, 0)
            data_relocations[section].append((offset, len(symbols) - 1, sc.DIR32))
        else:
            target_section, target_offset = data_location(target)
            struct.pack_into("<I", sections[section], offset, target_offset)
            data_relocations[section].append((offset, index[target_section], sc.DIR32))
    obj = out / "kotor_vc7_math.obj"
    obj.write_bytes(sc.write([sc.SectionSpec(".text$mn", bytes(code), text_relocations),
                              sc.SectionSpec(".rdata", bytes(sections[".rdata"]), data_relocations[".rdata"],
                                             0x40300040),
                              sc.SectionSpec(".data", bytes(sections[".data"]), data_relocations[".data"],
                                             0xC0300040)], symbols))
    segments = {".rdata": "KV_RDATA", ".data": "KV_DATA"}
    listing = [f"{names.get(s, f'D_{s:08X}')} . . . . . . . . . . . .\tByte\t {offset:08X}\t{segments[section]}"
               for s, (section, offset) in sorted((a, data_location(a)) for a in sorted(labelled))]
    lst = out / "kotor_vc7_math.lst"
    lst.write_text("\n".join(["Symbols:", "", *listing]) + "\n")
    return obj, lst
