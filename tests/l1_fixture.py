"""x86 code for the L1 math transcription to translate, placed at its entry points.

gen_l1_asm.py walks the code reachable from the VC7 math entry points. The
real routines are original code, so the synthetic executable carries short
stand-ins instead: one entry exercises every translation rule, the others
return at once. Pointer slots point at code and at data, as in the original.
"""
from __future__ import annotations

import struct

RICH_ENTRY = 0x006FC1B0                 # _pow; 64 bytes before the next entry
RETURN_ONLY = (0x006FC0E0, 0x006FC0F4, 0x006FCFF0, 0x006FD004, 0x006FC1F0, 0x006FC750,
               0x006FC880, 0x006FE517, 0x007043C1, 0x006FC412)
ERRNO_CALL, INTERNAL_CALL = 0x006FFE78, 0x006FC412
POINTER_SLOTS = [0x007A316C] + [0x007A32DC + 8 * i for i in range(29)]


def rich_entry() -> bytes:
    code = bytearray()

    def emit(hex_bytes: str) -> None:
        code.extend(bytes.fromhex(hex_bytes))

    def rel32(target: int, length: int) -> bytes:
        return struct.pack("<i", target - (RICH_ENTRY + len(code) + length))

    emit("9b")                          # wait -> fwait
    emit("d9442400")                    # fld dword ptr [esp+0]: zero disp8 kept as db
    emit("dd05e8ed7300")                # fld qword ptr [0073EDE8]: data label
    emit("d8c1")                        # fadd st, st(1)
    emit("dec1")                        # faddp st(1), st
    emit("dcc9")                        # fmul st(1), st
    emit("84cd")                        # test ch, cl: operands written reversed
    emit("b8f8ed7300")                  # mov eax, OFFSET D_0073EDF8
    emit("a180317a00")                  # mov eax, [___security_cookie]
    emit("dd30")                        # fnsave [eax]
    emit("6a01")                        # push 1
    code += b"\xe8" + rel32(ERRNO_CALL, 5)          # call __errno (rebound boundary)
    code += b"\xe8" + rel32(INTERNAL_CALL, 5)       # call kv_test_whether_TOS_is_int
    ret = RICH_ENTRY + len(code) + 2 + 6
    code += b"\x74" + bytes([ret - (RICH_ENTRY + len(code) + 2)])   # je SHORT
    code += b"\x0f\x85" + rel32(ret, 6)                              # jne NEAR PTR
    emit("c3")
    assert RICH_ENTRY + len(code) <= 0x006FC1F0
    return bytes(code)


def patches() -> dict[int, bytes]:
    found = {RICH_ENTRY: rich_entry(), **dict.fromkeys(RETURN_ONLY, b"\xc3")}
    for k, slot in enumerate(POINTER_SLOTS):
        found[slot] = struct.pack("<I", 0x006FC0E0 if k % 2 else 0x0073EDE8)
    return found
