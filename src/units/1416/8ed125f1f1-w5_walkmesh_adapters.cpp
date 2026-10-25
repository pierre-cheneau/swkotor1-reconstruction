// W5 support adapters: native __fastcall exports over EXISTING host-Memory bodies (no new address credit).
//  * 00581530 CSWRoomSurfaceMesh::CheckAABBWalkable -> kotor::server_mainloop::mesh_ground_ray_00581530
//    (room_ground_ray.cpp, in 1115 as 1062-room_ground_ray.obj).
//  * 00583E30 CSWRoomSurfaceMesh::ComputeClippedLineSegment -> kotor::server_mainloop::mesh_segment_clip_00583e30
//    (mesh_segment_clip.cpp, in 1115 as 1071-mesh_segment_clip.obj) with the retained native numeric ports
//    from mesh_segment_clip_native.inc (included unchanged, read-only include path).
// The original thiscall by-value Vector arguments are the callee's own stack copies; the adapters pass the
// addresses of their by-value parameters, which the host bodies may modify exactly like the originals.
#include "4f99fc5850-w5_walkmesh.hpp"
#include "f4c2ba3794-room_ground_ray.hpp"      // server-runtime-source/source (read-only)
#include "a26f5ad031-mesh_segment_clip.hpp"    // server-runtime-source/source (read-only)
#include "8fe63eea15-room_valid_helpers.hpp"   // server-runtime-source/source (read-only): 00579680 / 00579420 host bodies
#include "4f622be2f2-w5_area_direct_line.hpp"
#include "4c7ad9327a-object_position.hpp"      // server-runtime-source/source (read-only): 004CD170 host body

extern "C" std::uint32_t __cdecl aurora_vector_equal_004aa980(const void* left, const void* right);
extern "C" int __fastcall kotor_fastcall_00581150(void* mesh, void*, int edge);   // CSWRoomSurfaceMesh::GetAdjacentSecond (src/bridge)

namespace {
using namespace kotor::server_mainloop;
void* ptr(Word x) { return reinterpret_cast<void*>(x); }
Word word(void* x) { return reinterpret_cast<Word>(x); }

// Direct x86 memory (no address remapping: these bodies only touch mesh/stack objects).
struct DirectMemory final : Memory {
    Word read(Word a) override { Word v; std::memcpy(&v, ptr(a), 4); return v; }
    void write(Word a, Word v) override { std::memcpy(ptr(a), &v, 4); }
    Word read16(Word a) override { std::uint16_t v; std::memcpy(&v, ptr(a), 2); return v; }
    void write16(Word a, Word v) override { const std::uint16_t s = static_cast<std::uint16_t>(v); std::memcpy(ptr(a), &s, 2); }
    Word read8(Word a) override { return *static_cast<const unsigned char*>(ptr(a)); }
};

struct GroundRay final : RoomGroundRayPorts {
    Word node(Word a, Word b, Word c, Word d, Word e) override {
        return static_cast<Word>(surface_mesh_check_aabb_node_00580920(ptr(a), nullptr, static_cast<w5::I>(b),
            static_cast<w5::Vector*>(ptr(c)), static_cast<w5::Vector*>(ptr(d)), static_cast<w5::SurfaceHit*>(ptr(e))));
    }
    void normalize(Word a, Word b) override { normalize_004ab180(static_cast<w5::Vector*>(ptr(a)), static_cast<w5::Vector*>(ptr(b))); }
    void scale(Word a, Word b) override { vector_scale_assign_004a97a0(static_cast<w5::Vector*>(ptr(a)), nullptr, w5::flt(b)); }
    void add(Word a, Word b) override { vector_add_assign_004a9740(static_cast<w5::Vector*>(ptr(a)), nullptr, static_cast<w5::Vector*>(ptr(b))); }
    // 005795AA..F9 (CSWSRoom::CheckAABBWalkable tail): hit += room position (+4) + mesh offset (+20), x87 order:
    // x/y keep the 53-bit sum of the first add; z's first sum is stored as a dword ([esp+18]) before the second add.
    // Only room_ground_ray_00579560 calls this port; the original arithmetic is given instead of a trap.
    void world_point(Word d, Word a, Word b) override {
        float dst[3], p[3], q[3];
        std::memcpy(dst, ptr(d), 12); std::memcpy(p, ptr(a), 12); std::memcpy(q, ptr(b), 12);
        const double x = static_cast<double>(dst[0]) + static_cast<double>(p[0]);     // 005795AA..B0
        const double y = static_cast<double>(dst[1]) + static_cast<double>(p[1]);     // 005795B6..BC
        const float z = static_cast<float>(static_cast<double>(dst[2]) + static_cast<double>(p[2])); // 005795C4..CA
        const float out[3] = {static_cast<float>(x + static_cast<double>(q[0])),      // 005795D0..D2
                              static_cast<float>(y + static_cast<double>(q[1])),      // 005795D6..D9
                              static_cast<float>(static_cast<double>(z) + static_cast<double>(q[2]))}; // 005795DD..EE
        std::memcpy(ptr(d), out, 12);                                                 // 005795E4..F9
    }
};

// 00579680 tail-jumps to CSWCollisionMesh::NoNonWalkPolys 005972B0 (world-los native) with *low/*high/*radius.
struct RoomSweep final : RoomValidHelperPorts {
    Word collision(Word mesh, Word a, Word b, Word low, Word high, Word radius, Word hit) override {
        return static_cast<Word>(collision_mesh_no_nonwalk_polys_005972b0(ptr(mesh), nullptr, static_cast<w5::Vector*>(ptr(a)),
            static_cast<w5::Vector*>(ptr(b)), w5::flt(low), w5::flt(high), w5::flt(radius), ptr(hit)));
    }
};
#include "d0e971de24-mesh_segment_clip_native.inc"   // NativeMeshSegmentClip + retained x87 numeric providers (unchanged)
}  // namespace

extern "C" {
U __fastcall w5_adapt_check_aabb_walkable_00581530(void* mesh, void*, Vector from, Vector to, SurfaceHit* hit) {
    DirectMemory m; GroundRay p;
    alignas(4) unsigned char scratch[0x18];   // (1,1,0) direction + scaled step, as the original [esp+..] locals
    return mesh_ground_ray_00581530(m, p, word(mesh), word(&from), word(&to), word(hit), word(scratch));
}
U __fastcall w5_adapt_compute_clipped_line_segment_00583e30(void* mesh, void*, Vector from, Vector to,
    Vector* clip_from, Vector* clip_to, U* edge) {
    DirectMemory m; NativeMeshSegmentClip p;
    alignas(8) unsigned char scratch[0x74];   // same scratch size as the existing area_safe_valid ports
    return mesh_segment_clip_00583e30(m, p, word(mesh), word(&from), word(&to), word(clip_from), word(clip_to),
        word(edge), word(scratch));
}
I __fastcall w5_adapt_room_no_nonwalk_polys_00579680(void* room, void*, Vector* a, Vector* b, const float* low,
    const float* high, const float* radius, void* walk_hit) {
    DirectMemory m; RoomSweep p;
    return static_cast<I>(room_sweep_00579680(m, p, word(room), word(a), word(b), word(const_cast<float*>(low)),
        word(const_cast<float*>(high)), word(const_cast<float*>(radius)), word(walk_hit)));
}
I __fastcall w5_adapt_room_adjacent_00579420(void* room, void*, U edge) {
    DirectMemory m;                                                  // 00579420: the room's surface mesh at +3Ch
    return static_cast<I>(kotor_fastcall_00581150(ptr(m.read(word(room) + 0x3c)), nullptr, static_cast<int>(edge)));
}
// 004CD170 CSWSObject::SetOrientation: W1's 0886 wrapper w1x_object_set_orientation_004cd170 (ruling; the W5
// adapter over the same 1018 host body is withdrawn).
}
