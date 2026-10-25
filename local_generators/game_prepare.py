"""D13: game-prepare/prepare_image_data.cpp from the pinned original.

The generated file holds original .data/.rdata bytes and must NEVER be
committed. IMAGE-CHECK.json beside it records the address, file offset,
length and sha256 of every emitted value.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

from .original import OriginalImage, sha256, write_text

# (field, address, size, kind): kind u unsigned, u64, i signed, c char, f zero float.
WINMAIN = (('quit', 0x7a39b8, 4, 'u'), ('argc', 0x833bfc, 4, 'u'), ('argv', 0x833c00, 4, 'u'),
           ('callback', 0x78e39c, 4, 'u'), ('priority_changed', 0x7a3ca4, 4, 'u'), ('priority_mode', 0x7a3ca0, 4, 'u'),
           ('width', 0x78d1d4, 4, 'u'), ('depth', 0x78d1e0, 1, 'c'), ('headlight', 0x7a3c50, 4, 'u'),
           ('frame_word', 0x7a3c5c, 4, 'u'), ('bias', 0x7b9274, 4, 'f'), ('client_ms', 0x7a3c7c, 4, 'f'),
           ('server_ms', 0x7a3c80, 4, 'f'), ('cap', 0x7a3c64, 4, 'f'), ('start', 0x7a39c8, 8, 'u64'),
           ('end', 0x7a39b0, 8, 'u64'), ('cap_start', 0x7a3c68, 8, 'u64'), ('cap_end', 0x7a3c70, 8, 'u64'),
           ('debug', 0x7a3a40, 4, 'u'), ('deferred', 0x7a3a48, 4, 'u'), ('position', 0x7a3a44, 4, 'u'),
           ('text', 0x7a3a4c, 1, 'c'), ('swapping', 0x7a3c9c, 4, 'u'), ('sleep_enabled', 0x7a3c58, 4, 'u'),
           ('sleep_ms', 0x78d1e8, 4, 'u'), ('mouse_trails', 0x7a3ca8, 4, 'i'), ('screen_saver', 0x7a3cac, 4, 'i'))
SHELL_ENABLES = (0x78d454, 0x78d474, 0x78d45c, 0x78d458, 0x78d448, 0x78d44c)
SHELL_STRINGS = (('graphics_version', 0x7a3cc4), ('title', 0x7a3ccc), ('caption_prefix', 0x7a3cdc))
REINIT = (('guard', 0x7a3a3c), ('mode', 0x78d1dc))
MESSAGE = (('movie_fullscreen', 0x7a3a10), ('movie_inactive', 0x7a3a18), ('focus', 0x78d1d0),
           ('last_tick', 0x7a3c4c), ('movie_window', 0x7a3cf8), ('flag', 0x8338f0))
# 007A3A08 (Tlk) is owned by lane-tlk (D44), 007A39EC/0074C5F8/00832894 by lane-sound (D25).
CELLS = (('void*', 'kotor_cell_interface_007a39f0', 0x7a39f0, 4, '005F8B14'),
         ('void*', 'kotor_cell_tlk_client_007a3a0c', 0x7a3a0c, 4, '005F89BB'),
         ('std::uint32_t', 'kotor_cell_allow_windowed_007a3a30', 0x7a3a30, 4, '005F873F'),
         ('unsigned char', 'kotor_cell_logging_008328e1', 0x8328e1, 1, '005F88BA'),
         ('void*', 'kotor_cell_rules_007a3a28', 0x7a3a28, 4, '005F522A/005F5266'))
# Original .rdata strings consumed by lane-prepare units (VA, consumer evidence).
STRINGS = ((0x73d648, 'ReInitAurora 0040396A/00403AAC'), (0x73d6ac, 'ReInitAurora 00403955/00403A97'),
           (0x73d6c0, 'ReInitAurora 00403942'), (0x73d694, 'ReInitAurora 00403A89'),
           (0x73d6e0, 'ReInitAurora 0040389F (FindWindowA class)'),
           (0x73d634, 'initCinematicBackdrop 00401A8A'), (0x73d62c, 'initCinematicBackdrop 00401AC2'),
           (0x73d610, 'initCinematicBackdrop 00401B2B/00401B30'),
           (0x752db0, 'CSWGuiLoadScreen 0067A79A'), (0x752d78, 'CSWGuiLoadScreen 0067A910'),
           (0x748cb4, 'CSWRules 00552CFC'), (0x746f28, 'CSWRules 00552D33'), (0x746f38, 'CSWRules 00552D88'),
           (0x748ca8, 'CSWRules 00552F60'), (0x7451d4, 'CSWRules 00552F90'),
           (0x748a24, 'CSWRules::LoadDifficultyInfo 00550C92'))

TEMPLATE = '''// GENERATED - NEVER COMMIT (D13). Produced by game-prepare/tools/gen_prepare_image_data.py
// from swkotor.exe sha256 %s; every value's address, file offset,
// length and sha256 are in IMAGE-CHECK.json next to this file.
#include "game_prepare_producers.h"
#include "prepare_image_data.h"
#include <cstddef>
#include <cstdint>
// Positional initializers below are only valid for the D25 Initial layout
// (no disable_sound member): a header mismatch must fail to compile.
static_assert(offsetof(kotor_menu_live_views::WinMainInitial, priority_changed) == 16, "D25 WinMainInitial layout");
static_assert(sizeof(kotor_menu_live_views::ShellInitial) == 48, "ShellInitial layout");

extern "C" {
const kotor_menu_live_views::WinMainInitial kotor_image_winmain_initial = {
%s
};
const kotor_menu_live_views::ShellInitial kotor_image_shell_initial = {
%s,
%s
};
const kotor_menu_live_views::ReInitInitial kotor_image_reinit_initial = {
%s
};
const kotor_menu_live_views::MessageInitial kotor_image_message_initial = {
%s
};
kotor_window_shell::Constants kotor_shell_constants = {
    kotor_rdata_str_0078d200,
    {%s},
    %s, %s
};
%s
const char kotor_rdata_str_0078d200[%d] = {%s}; // InitGameApp 004040C0/004040F5 class name
%s
}
'''


def chars(data: bytes) -> str:
    return ', '.join('0x%02X' % c for c in data)


def game_prepare(image: OriginalImage, out: Path) -> None:
    check = []

    def record(field, va, data, offset, source, emitted, evidence=''):
        check.append({'field': field, 'address': '%08X' % va,
                      'file_offset': None if offset is None else '%X' % offset,
                      'length': len(data), 'image_bytes_le': data.hex(), 'sha256': sha256(data),
                      'source': source, 'emitted': emitted, 'evidence': evidence})

    def num(field, va, size, kind='u'):
        data, offset, source = image.read_loaded(va, size)
        value = int.from_bytes(data, 'little')
        if kind == 'f':
            if value != 0:
                raise ValueError('nonzero float needs bit-exact emission at %x' % va)
            literal = '0.0f'
        else:
            literal = {'u': '0x%Xu' % value, 'u64': '0x%XULL' % value, 'c': '%d' % value,
                       'i': '%d' % (value - (1 << 32) if value >= 1 << 31 else value)}[kind]
        record(field, va, data, offset, source, literal)
        return literal

    def cstr_bytes(va):
        data, offset, source = image.read_loaded(va, 256)
        text = data.split(b'\0')[0] + b'\0'
        record('string %08X' % va, va, text, offset, source, 'char[%d]' % len(text))
        return text

    winmain = ['    /* %-16s %08X */ %s' % (n, va, num('winmain.' + n, va, s, k)) for n, va, s, k in WINMAIN]
    strings = ['    /* %-16s %08X loader image, before staticInit */ {%s, %s}'
               % (n, va, num('shell.%s.data' % n, va, 4), num('shell.%s.length' % n, va + 4, 4))
               for n, va in SHELL_STRINGS]
    enables = ['    /* %08X */ %s' % (va, num('shell.enable_%06x' % va, va, 4)) for va in SHELL_ENABLES]
    reinit = ['    /* %-6s %08X */ %s' % (n, va, num('reinit.' + n, va, 4)) for n, va in REINIT]
    message = ['    /* %-16s %08X */ %s' % (n, va, num('message.' + n, va, 4)) for n, va in MESSAGE]
    render_class, offset, source = image.read_loaded(0x73d684, 14)
    record('shell_constants.render_class', 0x73d684, render_class, offset, source, 'char[14]', '00403708..00403736')
    top = cstr_bytes(0x78d200)
    blend_source = num('shell_constants.blend_source', 0x73f270, 4)
    blend_destination = num('shell_constants.blend_destination', 0x73f274, 4)
    cells = []
    for kind, name, va, size, evidence in CELLS:
        data, offset, source = image.read_loaded(va, size)
        value = int.from_bytes(data, 'little')
        if kind == 'void*' and value != 0:
            raise ValueError('nonzero pointer cell %x' % va)
        literal = 'nullptr' if kind == 'void*' else '0x%Xu' % value
        record(name, va, data, offset, source, literal, evidence)
        cells.append('%s %s = %s; // %08X (%s); %s' % (kind, name, literal, va, source, evidence))
    rdata = []
    for va, evidence in STRINGS:
        text = cstr_bytes(va)
        rdata.append('const char kotor_rdata_str_%08x[%d] = {%s}; // %s' % (va, len(text), chars(text), evidence))

    source_text = TEMPLATE % (image.sha256, ',\n'.join(winmain), ',\n'.join(strings), ',\n'.join(enables),
                              ',\n'.join(reinit), ',\n'.join(message), chars(render_class), blend_source,
                              blend_destination, '\n'.join(cells), len(top), chars(top), '\n'.join(rdata))
    out.mkdir(parents=True, exist_ok=True)
    write_text(out / 'prepare_image_data.cpp', source_text, '\r\n')
    write_text(out / 'NEVER-COMMIT.txt', 'Generated from original game bytes (D13). Do not commit, publish '
               'or copy into the repository.\n', '\r\n')
    write_text(out / '.gitignore', '*\n', '\r\n')
    write_text(out / 'IMAGE-CHECK.json', json.dumps({'exe_sha256': image.sha256, 'generator': os.path.abspath(__file__),
                                                     'values': check}, indent=1), '\r\n')
