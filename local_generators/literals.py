"""Original literals and small tables named by address or by a spec."""
from __future__ import annotations

import re
import struct
from pathlib import Path

from .original import OriginalImage, byte_list, load_spec, sha256, write_text

SHARED_EMPTY = 0x73D71C          # base_ini_reconstruction::warning_empty_0073d71c, defined elsewhere
CLASS_CHOICES = 0x007A2684
UPGRADE_SLOTS = 0x00756FB0
TEXTURE_LITERAL = 0x0073EE34


def is_ascii(data: bytes) -> bool:
    return all(b < 128 for b in data)


def resource_pairs(image: OriginalImage, output: Path) -> None:
    entries = load_spec("resource-pairs-spec.json")
    if len(entries) != 88:
        raise ValueError("Expected 88 ordered resource pairs")
    lines = ['#pragma section(".rdata$BRT", read)',
             '// Local ignored original-backed data; do not publish.',
             '#include "baseinternal.h"', '#include "warning_runtime.h"',
             'namespace base_resource_table {']
    for index, item in enumerate(entries):
        address, size = int(item["original_address"], 16), item["size"]
        if item["index"] != index or type(item["id"]) is not int or not 0 <= item["id"] <= 65535:
            raise ValueError("Invalid resource-pair ordering/ID")
        if type(size) is not int or not 1 <= size <= 4096:
            raise ValueError("Invalid resource literal size")
        data = image.read(address, size)
        if data[-1:] != b"\0" or b"\0" in data[:-1] or not is_ascii(data) or sha256(data) != item["span_sha256"]:
            raise ValueError("Original resource literal span mismatch")
        if index < 87:
            symbol = f"resource_literal_{address:08x}"
            if item["canonical_symbol"] != "base_resource_table::" + symbol or item["new_definition"] is not True:
                raise ValueError("Invalid resource literal owner")
            lines.append(f"extern const char {symbol}[] = {{{byte_list(data)}}};")
        elif (item["canonical_symbol"] != "base_ini_reconstruction::warning_empty_0073d71c"
              or item["new_definition"] is not False or data != b"\0" or address != SHARED_EMPTY):
            raise ValueError("Invalid shared empty-string owner")
    lines.append("extern const ResourcePair canonical_resource_pairs[88] = {")
    lines.extend(f"    {{{item['id']}u, {item['canonical_symbol']}}}," for item in entries)
    lines.extend(["};", '__declspec(allocate(".rdata$BRT")) const ResourcePair (&resource_pairs)[88] = '
                  "canonical_resource_pairs;", "}"])
    write_text(output, "\n".join(lines) + "\n", "\n")


def extra_data(image: OriginalImage, output: Path) -> None:
    for group in load_spec("extra-data-spec.json"):
        destination = (output / group["output"]).resolve()
        if not destination.is_relative_to(output):
            raise ValueError("Invalid generated output path")
        lines = ["// Generated locally from supported original; never publish.", *group["prefix"]]
        for item in group["objects"]:
            storage = item.get("storage", "extern const")
            if (not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", item["name"])
                    or item["type"] not in ("char", "unsigned char")
                    or storage not in ("const", "constexpr", "extern const")
                    or type(item["size"]) is not int or not 0 < item["size"] <= 4096
                    or item.get("align") not in (None, 4, 8)):
                raise ValueError("Invalid generated object metadata")
            data = image.read(int(item["va"], 16), item["size"])
            if item["type"] == "char" and not is_ascii(data):
                raise ValueError("Invalid original span")
            alignment = f"alignas({item['align']}) " if item.get("align") else ""
            lines.append(f"{alignment}{storage} {item['type']} {item['name']}[{item['size']}] = "
                         f"{{{byte_list(data, separator=',')}}};")
        destination.parent.mkdir(parents=True, exist_ok=True)
        write_text(destination, "\n".join([*lines, *group["suffix"]]) + "\n", "\r\n")


def class_choices(image: OriginalImage, output: Path) -> None:
    lines = ["// Original-backed local data; never publish.", '#include "m15a_gui.h"',
             'extern "C" KotorM15aClassChoice kotor_m15a_class_choices_007a2684[6] = {']
    lines.extend("    {" + ", ".join(f"{n}u" for n in record) + "},"
                 for record in struct.iter_unpack("<BBBBI", image.read(CLASS_CHOICES, 48)))
    write_text(output, "\n".join([*lines, "};"]) + "\n", "\r\n")


def upgrade_slots(image: OriginalImage, output: Path) -> None:
    lines = ["// Local original-backed data; never publish.", '#include "claude_upgrade.hpp"', "namespace upgrade {"]
    records = []
    for i, (kind, pointer, strref) in enumerate(struct.iter_unpack("<iII", image.read(UPGRADE_SLOTS, 16 * 12))):
        icon = image.cstring(pointer, limit=256)
        if not is_ascii(icon):
            raise ValueError("Invalid upgrade icon string")
        lines.append(f"static const char icon_{i}[] = {{{byte_list(icon)}}};")
        records.append(f"    {{{kind}, icon_{i}, {strref}u}},")
    lines.extend(["extern const SlotInfo k_slots_00756fb0[16] = {", *records, "};", "}"])
    write_text(output, "\n".join(lines) + "\n", "\r\n")


def texture_literal(image: OriginalImage, output: Path) -> None:
    data = image.read(TEXTURE_LITERAL, 12)
    if data[-1:] != b"\0" or not is_ascii(data):
        raise ValueError("Invalid original texture literal")
    write_text(output, "// Local original-backed data; never publish.\nnamespace texture_cells {\n"
               f"extern const char norm_cube_map_0073ee34[12] = {{{byte_list(data)}}};\n}}\n", "\r\n")
