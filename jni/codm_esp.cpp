#include "codm_esp.h"
#include "codm_attach.h"

#include "ImGui/imgui.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <utility>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kNeedles = 14;
const char *kNeedle[kNeedles] = {
    "get_MatchGame", "get_LocalPawn", "get_main", "get_LastPawnPos", "get_HeadPosition",
    "get_PlayerName", "EnemyPawns", "m_IsAlive", "m_IsBot", "m_LastPawnPos", "m_AttackableInfo",
    "m_Health", "m_MaxHealth", "m_HeadPosition",
};

enum {
    N_MATCH = 0,
    N_LOCAL,
    N_MAIN,
    N_LAST_M,
    N_HEAD_M,
    N_NAME_M,
    N_ENEMY,
    N_ALIVE,
    N_BOT,
    N_LAST_F,
    N_INFO,
    N_HP,
    N_MAX,
    N_HEAD_F
};

struct Range {
    uintptr_t start;
    uintptr_t end;
    bool exec;
    bool unity;
    bool meta;
};

struct Snap {
    int generation = 0;
    int attempts = 0;
    bool ready = false;
    pid_t pid = 0;
    uintptr_t matchSlot = 0;
    uintptr_t localSlot = 0;
    uintptr_t camSlot = 0;
    int enemyOff = -1;
    int aliveOff = -1;
    int botOff = -1;
    int posOff = -1;
    int headOff = -1;
    int nameOff = -1;
    int infoOff = -1;
    int hpOff = -1;
    int maxOff = -1;
    int baseField = 0x10;
    int baseIsNative = 1;
    int viewOff = -1;
    int projOff = -1;
    int mulOrder = 0;
    int matrixTries = 0;
    int matrixMiss = 0;
    char status[80] = "ESP resolving";
};

std::mutex g_mu;
Snap g_snap;
int g_gen = 0;

bool mem_read(pid_t pid, uintptr_t addr, void *buf, size_t n) {
    if (pid <= 0 || !addr || !buf || !n) return false;
    iovec local{buf, n};
    iovec remote{reinterpret_cast<void *>(addr), n};
    return syscall(SYS_process_vm_readv, pid, &local, 1, &remote, 1, 0) == (ssize_t)n;
}

bool ptr_ok(uintptr_t p) {
    return p >= 0x10000ULL && p < 0x800000000000ULL && (p & 7ULL) == 0;
}

bool addr_ok(uintptr_t p) { return p >= 0x10000ULL && p < 0x800000000000ULL; }

void elog(const char *fmt, ...) {
    char buf[240];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    attach_log_line(buf);
}

bool read_cstr(pid_t pid, uintptr_t addr, char *out, size_t cap) {
    if (!out || cap < 2 || !addr_ok(addr)) return false;
    char tmp[72];
    if (!mem_read(pid, addr, tmp, sizeof(tmp))) return false;
    tmp[sizeof(tmp) - 1] = 0;
    for (size_t i = 0; i < sizeof(tmp) - 1; ++i) {
        if (tmp[i] == 0) {
            if (i == 0 || i >= cap) return false;
            std::memcpy(out, tmp, i + 1);
            return true;
        }
        unsigned char c = (unsigned char)tmp[i];
        if (c < 32 || c > 126) return false;
    }
    return false;
}

bool class_name(pid_t pid, uintptr_t klass, char *name, size_t nameCap, char *ns, size_t nsCap) {
    if (name && nameCap) name[0] = 0;
    if (ns && nsCap) ns[0] = 0;
    if (!ptr_ok(klass)) return false;
    uintptr_t np = 0, nsp = 0;
    if (!mem_read(pid, klass + 0x10, &np, sizeof(np))) return false;
    if (!read_cstr(pid, np, name, nameCap)) return false;
    if (ns && nsCap && mem_read(pid, klass + 0x18, &nsp, sizeof(nsp)))
        read_cstr(pid, nsp, ns, nsCap);
    return true;
}

bool name_is(pid_t pid, uintptr_t klass, const char *want) {
    char name[64];
    if (!class_name(pid, klass, name, sizeof(name), nullptr, 0)) return false;
    return std::strcmp(name, want) == 0;
}

bool name_one_of(pid_t pid, uintptr_t klass, const char *const *names) {
    char name[64];
    if (!class_name(pid, klass, name, sizeof(name), nullptr, 0)) return false;
    for (int i = 0; names[i]; ++i)
        if (std::strcmp(name, names[i]) == 0) return true;
    return false;
}

bool decode_adrp(uint32_t w, uint64_t pc, int64_t &pageOut) {
    if ((w & 0x9F000000u) != 0x90000000u) return false;
    int64_t immhi = (int64_t)((w >> 5) & 0x7ffffu);
    int64_t immlo = (int64_t)((w >> 29) & 0x3u);
    int64_t imm = (immhi | (immlo << 19)) << 12;
    if (w & (1u << 23)) imm |= (int64_t)0xFFFFFFFFF0000000ULL;
    pageOut = (int64_t)(pc & ~0xFFFULL) + imm;
    return true;
}

bool is_branch(uint32_t w) {
    const uint32_t op = w & 0xFC000000u;
    if (op == 0x14000000u || op == 0x94000000u) return true;
    if ((w & 0xFFFFFC1Fu) == 0xD61F0000u || (w & 0xFFFFFC1Fu) == 0xD63F0000u) return true;
    return false;
}

bool is_ret(uint32_t w) { return (w & 0xFFFFFC1Fu) == 0xD65F0000u; }

bool in_exec(const std::vector<Range> &ranges, uintptr_t p) {
    if (!p || (p & 3ULL)) return false;
    for (const Range &r : ranges) {
        if (r.exec && p >= r.start && p < r.end) return true;
    }
    return false;
}

bool decode_static_slot(pid_t pid, uintptr_t func, uintptr_t &slot) {
    slot = 0;
    uint32_t ins[32] = {};
    if (!mem_read(pid, func, ins, sizeof(ins))) return false;
    uint64_t x[31] = {};
    bool have[31] = {};
    for (int i = 0; i < 32; ++i) {
        const uint32_t w = ins[i];
        if (is_ret(w) || is_branch(w)) break;
        int64_t page = 0;
        if (decode_adrp(w, func + (uint64_t)i * 4ull, page)) {
            const int rd = (int)(w & 31u);
            if (rd < 31) {
                x[rd] = (uint64_t)page;
                have[rd] = true;
            }
            continue;
        }
        if ((w & 0xFFC00000u) == 0x91000000u) {
            const int rd = (int)(w & 31u);
            const int rn = (int)((w >> 5) & 31u);
            const uint64_t imm = (uint64_t)((w >> 10) & 0xFFFu);
            if (rd < 31 && rn < 31 && have[rn]) {
                x[rd] = x[rn] + imm;
                have[rd] = true;
            }
            continue;
        }
        if ((w & 0xFFC00000u) == 0xF9400000u) {
            const int rt = (int)(w & 31u);
            const int rn = (int)((w >> 5) & 31u);
            const uint64_t off = (uint64_t)((w >> 10) & 0xFFFu) * 8ull;
            if (rn < 31 && have[rn]) {
                uintptr_t addr = (uintptr_t)(x[rn] + off);
                if (rt == 0) slot = addr;
                uintptr_t val = 0;
                if (rt < 31 && mem_read(pid, addr, &val, sizeof(val))) {
                    x[rt] = val;
                    have[rt] = true;
                }
            }
        }
    }
    uintptr_t probe = 0;
    return slot && mem_read(pid, slot, &probe, sizeof(probe));
}

int decode_vec3(pid_t pid, uintptr_t func) {
    uint32_t ins[24] = {};
    if (!mem_read(pid, func, ins, sizeof(ins))) return -1;
    int offs[8];
    int n = 0;
    for (int i = 0; i < 24; ++i) {
        const uint32_t w = ins[i];
        if (is_ret(w) || is_branch(w)) break;
        if ((w & 0xFFC00000u) != 0xBD400000u) continue;
        const int rn = (int)((w >> 5) & 31u);
        const int imm = (int)(((w >> 10) & 0xFFFu) * 4u);
        if (rn == 0 && n < 8) offs[n++] = imm;
    }
    for (int i = 0; i < n; ++i) {
        bool p4 = false, p8 = false;
        for (int j = 0; j < n; ++j) {
            if (offs[j] == offs[i] + 4) p4 = true;
            if (offs[j] == offs[i] + 8) p8 = true;
        }
        if (p4 && p8 && offs[i] >= 0x10 && offs[i] < 0x2000) return offs[i];
    }
    return -1;
}

int decode_ptr_field(pid_t pid, uintptr_t func) {
    uint32_t ins[16] = {};
    if (!mem_read(pid, func, ins, sizeof(ins))) return -1;
    bool x0this = true;
    int found = -1;
    for (int i = 0; i < 16; ++i) {
        const uint32_t w = ins[i];
        if (is_ret(w) || is_branch(w)) break;
        int64_t page = 0;
        if (decode_adrp(w, func + (uint64_t)i * 4ull, page)) {
            if ((w & 31u) == 0) x0this = false;
            continue;
        }
        if ((w & 0xFFC00000u) == 0x91000000u) {
            if ((w & 31u) == 0) x0this = false;
            continue;
        }
        if ((w & 0xFFC00000u) == 0xF9400000u) {
            const int rt = (int)(w & 31u);
            const int rn = (int)((w >> 5) & 31u);
            const int imm = (int)(((w >> 10) & 0xFFFu) * 8u);
            if (rt == 0 && rn == 0 && x0this) found = imm;
            if (rt == 0) x0this = false;
        }
    }
    if (found < 0x10 || found >= 0x2000 || (found & 7)) return -1;
    return found;
}

void log_code(pid_t pid, const char *label, uintptr_t func) {
    uint32_t ins[4] = {};
    if (!func || !mem_read(pid, func, ins, sizeof(ins))) {
        elog("[esp] %s code missing", label);
        return;
    }
    elog("[esp] %s code %08x %08x %08x %08x", label, ins[0], ins[1], ins[2], ins[3]);
}

struct StrKey {
    uintptr_t addr;
    int id;
};

bool collect_ranges(pid_t pid, std::vector<Range> &out, int &unityN, int &metaN) {
    unityN = metaN = 0;
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;
    char line[768];
    while (std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0;
        char perms[8] = {};
        char file[512] = {};
        file[0] = 0;
        if (std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %511[^\n]", &start, &end, perms, file) < 3)
            continue;
        if (end <= start) continue;
        const bool unity = std::strstr(file, "libunity.so") != nullptr;
        const bool meta = std::strstr(file, "global-metadata.dat") != nullptr;
        if (!unity && !meta) continue;
        if ((size_t)(end - start) > 256u * 1024u * 1024u) continue;
        Range r{};
        r.start = (uintptr_t)start;
        r.end = (uintptr_t)end;
        r.exec = perms[2] == 'x';
        r.unity = unity;
        r.meta = meta;
        out.push_back(r);
        if (unity) ++unityN;
        if (meta) ++metaN;
    }
    std::fclose(fp);
    return !out.empty();
}

uintptr_t unity_file_base(pid_t pid) {
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return 0;
    char line[768];
    uintptr_t base = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, off = 0;
        char file[512] = {};
        if (std::sscanf(line, "%lx-%*x %*s %lx %*s %*s %511[^\n]", &start, &off, file) < 3) continue;
        if (!std::strstr(file, "libunity.so")) continue;
        if (off == 0) {
            base = (uintptr_t)start;
            break;
        }
    }
    std::fclose(fp);
    return base;
}

uintptr_t follow_b(pid_t pid, uintptr_t func) {
    uint32_t w = 0;
    if (!func || !mem_read(pid, func, &w, sizeof(w))) return func;
    if ((w & 0xFC000000u) != 0x14000000u) return func;
    int32_t imm = (int32_t)(w & 0x03FFFFFFu);
    if (imm & 0x02000000) imm |= ~0x03FFFFFF;
    return func + (uintptr_t)(imm * 4);
}

void search_strings(pid_t pid, const std::vector<Range> &ranges, std::vector<StrKey> &keys) {
    int found[kNeedles] = {};
    std::vector<uint8_t> buf(65536 + 48);
    size_t scanned = 0;
    for (const Range &r : ranges) {
        if (r.exec || scanned > 192u * 1024u * 1024u) continue;
        if (r.unity == false && r.meta == false) continue;
        size_t keep = 0;
        for (uintptr_t addr = r.start; addr < r.end && scanned < 192u * 1024u * 1024u;) {
            size_t n = (size_t)(r.end - addr);
            if (n > 65536) n = 65536;
            if (!mem_read(pid, addr, buf.data() + keep, n)) break;
            const size_t total = keep + n;
            scanned += n;
            for (int id = 0; id < kNeedles; ++id) {
                if (found[id] >= 6) continue;
                const char *needle = kNeedle[id];
                const size_t len = std::strlen(needle);
                if (total < len + 1) continue;
                for (size_t i = (keep && addr != r.start) ? keep : 0; i + len < total; ++i) {
                    if (i && buf[i - 1] != 0) continue;
                    if (std::memcmp(buf.data() + i, needle, len) != 0) continue;
                    if (buf[i + len] != 0) continue;
                    keys.push_back(StrKey{addr - keep + i, id});
                    ++found[id];
                    if (found[id] >= 6) break;
                }
            }
            keep = 32;
            if (total >= keep) std::memmove(buf.data(), buf.data() + total - keep, keep);
            addr += n;
        }
    }
    elog("[esp] strings match=%d local=%d main=%d enemy=%d alive=%d pos=%d", found[N_MATCH],
         found[N_LOCAL], found[N_MAIN], found[N_ENEMY], found[N_ALIVE],
         found[N_LAST_F] + found[N_LAST_M]);
}

void search_ptrs(pid_t pid, const std::vector<Range> &ranges, std::vector<StrKey> &keys,
                 std::vector<uintptr_t> hits[kNeedles]) {
    if (keys.empty()) return;
    std::sort(keys.begin(), keys.end(), [](const StrKey &a, const StrKey &b) { return a.addr < b.addr; });
    std::vector<uint8_t> buf(65536);
    size_t scanned = 0;
    for (const Range &r : ranges) {
        if (!r.unity || r.exec || scanned > 160u * 1024u * 1024u) continue;
        uintptr_t addr = r.start;
        if (addr & 7) addr = (addr + 7) & ~7ull;
        while (addr + 8 <= r.end && scanned < 160u * 1024u * 1024u) {
            size_t n = (size_t)(r.end - addr);
            if (n > 65536) n = 65536;
            n &= ~7u;
            if (n < 8) break;
            if (!mem_read(pid, addr, buf.data(), n)) {
                addr += 65536;
                continue;
            }
            scanned += n;
            for (size_t i = 0; i + 8 <= n; i += 8) {
                uintptr_t val = 0;
                std::memcpy(&val, buf.data() + i, 8);
                auto it = std::lower_bound(keys.begin(), keys.end(), val,
                                            [](const StrKey &s, uintptr_t v) { return s.addr < v; });
                if (it == keys.end() || it->addr != val) continue;
                const int id = it->id;
                if (hits[id].size() < 160) hits[id].push_back(addr + i);
            }
            addr += n;
        }
    }
    elog("[esp] hits match=%zu local=%zu main=%zu enemy=%zu alive=%zu posf=%zu", hits[N_MATCH].size(),
         hits[N_LOCAL].size(), hits[N_MAIN].size(), hits[N_ENEMY].size(), hits[N_ALIVE].size(),
         hits[N_LAST_F].size());
}

uintptr_t method_slot(pid_t pid, const std::vector<Range> &ranges, const std::vector<uintptr_t> &hits,
                      const char *cls, const char *ns, const char *label) {
    uintptr_t fns[4] = {};
    int nfn = 0;
    char seenNs[64] = {};
    for (uintptr_t hit : hits) {
        uintptr_t klass = 0;
        if (!mem_read(pid, hit + 8, &klass, sizeof(klass)) || !ptr_ok(klass)) continue;
        char name[64], nspace[64];
        if (!class_name(pid, klass, name, sizeof(name), nspace, sizeof(nspace))) continue;
        if (std::strcmp(name, cls) != 0) continue;
        if (ns && std::strcmp(nspace, ns) != 0) {
            if (!seenNs[0]) std::snprintf(seenNs, sizeof(seenNs), "%s", nspace);
            continue;
        }
        const int nameOffs[] = {16, 24, 32};
        for (int nameOff : nameOffs) {
            if (hit < (uintptr_t)nameOff) continue;
            uintptr_t fn = 0;
            if (!mem_read(pid, hit - (uintptr_t)nameOff, &fn, sizeof(fn))) continue;
            if (!in_exec(ranges, fn)) continue;
            bool dup = false;
            for (int i = 0; i < nfn; ++i)
                if (fns[i] == fn) dup = true;
            if (!dup && nfn < 4) fns[nfn++] = fn;
        }
    }
    if (!nfn && ns && seenNs[0]) {
        elog("[esp] %s class %s ns is %s", label, cls, seenNs);
        return method_slot(pid, ranges, hits, cls, nullptr, label);
    }
    for (int i = 0; i < nfn; ++i) {
        uintptr_t slot = 0;
        if (decode_static_slot(pid, fns[i], slot)) return slot;
    }
    if (nfn) log_code(pid, label, fns[0]);
    else elog("[esp] %s method not found", label);
    return 0;
}

int method_vec(pid_t pid, const std::vector<Range> &ranges, const std::vector<uintptr_t> &hits,
               const char *cls) {
    for (uintptr_t hit : hits) {
        uintptr_t klass = 0;
        if (!mem_read(pid, hit + 8, &klass, sizeof(klass)) || !name_is(pid, klass, cls)) continue;
        const int nameOffs[] = {16, 24, 32};
        for (int nameOff : nameOffs) {
            if (hit < (uintptr_t)nameOff) continue;
            uintptr_t fn = 0;
            if (!mem_read(pid, hit - (uintptr_t)nameOff, &fn, sizeof(fn))) continue;
            if (!in_exec(ranges, fn)) continue;
            const int off = decode_vec3(pid, fn);
            if (off > 0) return off;
        }
    }
    return -1;
}

int method_ptr_off(pid_t pid, const std::vector<Range> &ranges, const std::vector<uintptr_t> &hits,
                   const char *cls) {
    for (uintptr_t hit : hits) {
        uintptr_t klass = 0;
        if (!mem_read(pid, hit + 8, &klass, sizeof(klass)) || !name_is(pid, klass, cls)) continue;
        const int nameOffs[] = {16, 24, 32};
        for (int nameOff : nameOffs) {
            if (hit < (uintptr_t)nameOff) continue;
            uintptr_t fn = 0;
            if (!mem_read(pid, hit - (uintptr_t)nameOff, &fn, sizeof(fn))) continue;
            if (!in_exec(ranges, fn)) continue;
            const int off = decode_ptr_field(pid, fn);
            if (off > 0) return off;
        }
    }
    return -1;
}

int field_off(pid_t pid, const std::vector<uintptr_t> &hits, const char *const *classes, int align,
              const char *label) {
    int onlyOff = -1;
    int valids = 0;
    char onlyName[64] = {};
    for (uintptr_t hit : hits) {
        uintptr_t parent = 0;
        int32_t off = 0;
        if (!mem_read(pid, hit + 16, &parent, sizeof(parent))) continue;
        if (!mem_read(pid, hit + 24, &off, sizeof(off))) continue;
        if (!ptr_ok(parent) || off < 0x10 || off >= 0x2000) continue;
        if (align > 1 && (off % align) != 0) continue;
        char name[64];
        if (!class_name(pid, parent, name, sizeof(name), nullptr, 0)) continue;
        ++valids;
        if (valids == 1) {
            onlyOff = off;
            std::snprintf(onlyName, sizeof(onlyName), "%s", name);
        }
        if (name_one_of(pid, parent, classes)) return off;
    }
    if (valids == 1) {
        elog("[esp] %s using %s+%#x", label, onlyName, onlyOff);
        return onlyOff;
    }
    if (onlyName[0]) elog("[esp] %s parent=%s off=%#x valids=%d", label, onlyName, onlyOff, valids);
    else elog("[esp] %s field not found", label);
    return -1;
}

bool finite3(float x, float y, float z) {
    return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::fabs(x) < 200000.f &&
           std::fabs(y) < 200000.f && std::fabs(z) < 200000.f && !(x == 0.f && y == 0.f && z == 0.f);
}

bool is_proj(const float *f) {
    if (!(f[0] > 0.05f && f[0] < 20.f && f[5] > 0.05f && f[5] < 20.f)) return false;
    if (std::fabs(f[11] + 1.f) > 0.08f && std::fabs(f[11] - 1.f) > 0.08f) return false;
    if (std::fabs(f[15]) > 0.08f) return false;
    if (std::fabs(f[1]) > 0.08f || std::fabs(f[4]) > 0.08f) return false;
    if (std::fabs(f[14]) < 0.01f) return false;
    return true;
}

bool len1(float x, float y, float z) {
    const float l = std::sqrt(x * x + y * y + z * z);
    return l > 0.85f && l < 1.15f;
}

bool is_view(const float *f) {
    if (!len1(f[0], f[1], f[2]) || !len1(f[4], f[5], f[6]) || !len1(f[8], f[9], f[10])) return false;
    return std::fabs(f[15] - 1.f) < 0.08f;
}

void mul4(const float *a, const float *b, float *o) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            o[c * 4 + r] = a[0 * 4 + r] * b[c * 4 + 0] + a[1 * 4 + r] * b[c * 4 + 1] +
                           a[2 * 4 + r] * b[c * 4 + 2] + a[3 * 4 + r] * b[c * 4 + 3];
        }
    }
}

bool project(const float *vp, float x, float y, float z, int width, int height, float &sx, float &sy) {
    const float w = vp[3] * x + vp[7] * y + vp[11] * z + vp[15];
    if (w < 0.05f) return false;
    const float cx = vp[0] * x + vp[4] * y + vp[8] * z + vp[12];
    const float cy = vp[1] * x + vp[5] * y + vp[9] * z + vp[13];
    sx = (cx / w + 1.f) * 0.5f * (float)width;
    sy = (float)height - (cy / w + 1.f) * 0.5f * (float)height;
    return std::isfinite(sx) && std::isfinite(sy);
}

bool upright(const float *vp, float x, float y, float z, int width, int height) {
    float x1, y1, x2, y2;
    if (!project(vp, x, y, z, width, height, x1, y1)) return false;
    if (!project(vp, x, y + 1.7f, z, width, height, x2, y2)) return false;
    if (!(y2 < y1 - 1.f)) return false;
    if (std::fabs(x1) > 20000.f || std::fabs(y1) > 20000.f) return false;
    return true;
}

bool find_matrices(const uint8_t *buf, size_t n, float x, float y, float z, int width, int height,
                   int &viewOff, int &projOff, int &order) {
    for (size_t po = 0x20; po + 64 <= n; po += 4) {
        const float *proj = reinterpret_cast<const float *>(buf + po);
        if (!is_proj(proj)) continue;
        size_t voBegin = po > 0x180 ? po - 0x180 : 0x20;
        size_t voEnd = po + 0x180;
        if (voEnd + 64 > n) voEnd = n - 64;
        for (size_t vo = voBegin; vo + 64 <= voEnd; vo += 4) {
            if (vo + 64 > po && po + 64 > vo) continue;
            const float *view = reinterpret_cast<const float *>(buf + vo);
            if (!is_view(view)) continue;
            float vp[16];
            mul4(proj, view, vp);
            if (upright(vp, x, y, z, width, height)) {
                viewOff = (int)vo;
                projOff = (int)po;
                order = 0;
                return true;
            }
            mul4(view, proj, vp);
            if (upright(vp, x, y, z, width, height)) {
                viewOff = (int)vo;
                projOff = (int)po;
                order = 1;
                return true;
            }
        }
    }
    return false;
}

void paint_status(ImDrawList *draw, int width, const char *text) {
    if (!draw || !text || !text[0] || width < 50) return;
    ImVec2 sz = ImGui::CalcTextSize(text);
    ImVec2 pos((width - sz.x) * 0.5f, 52.f);
    draw->AddText(ImVec2(pos.x + 1.f, pos.y + 1.f), IM_COL32(0, 0, 0, 220), text);
    draw->AddText(pos, IM_COL32(255, 255, 255, 230), text);
}

} // namespace

void codm_esp_reset() {
    std::lock_guard<std::mutex> lock(g_mu);
    g_snap = Snap{};
    std::snprintf(g_snap.status, sizeof(g_snap.status), "ESP resolving");
    g_gen++;
    g_snap.generation = g_gen;
}

void codm_esp_resolve(pid_t pid) {
    if (pid <= 0) return;
    int gen = 0;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        if (g_snap.ready && g_snap.pid == pid) return;
        if (g_snap.pid != pid) {
            g_snap = Snap{};
            g_gen++;
            g_snap.generation = g_gen;
            g_snap.pid = pid;
        }
        if (g_snap.attempts >= 3) return;
        g_snap.attempts++;
        gen = g_snap.generation;
        std::snprintf(g_snap.status, sizeof(g_snap.status), "ESP resolving");
    }
    int attempt = 0;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        attempt = g_snap.attempts;
    }

    std::vector<Range> ranges;
    int unityN = 0, metaN = 0;
    if (!collect_ranges(pid, ranges, unityN, metaN)) {
        elog("[esp] maps unreadable");
        return;
    }
    elog("[esp] ranges unity=%d meta=%d", unityN, metaN);
    std::vector<StrKey> keys;
    search_strings(pid, ranges, keys);
    std::vector<uintptr_t> hits[kNeedles];
    search_ptrs(pid, ranges, keys, hits);

    Snap next{};
    next.pid = pid;
    next.generation = gen;
    static const char *kPawn[] = {"Pawn", "PlayerPawn", nullptr};
    static const char *kBase[] = {"BaseGame", "MatchGame", nullptr};
    static const char *kInfo[] = {"AttackableTargetInfo", nullptr};
    static const char *kTarget[] = {"AttackableTarget", "Pawn", nullptr};
    next.matchSlot = method_slot(pid, ranges, hits[N_MATCH], "GamePlay", nullptr, "get_MatchGame");
    next.localSlot = method_slot(pid, ranges, hits[N_LOCAL], "GamePlay", nullptr, "get_LocalPawn");
    next.camSlot = method_slot(pid, ranges, hits[N_MAIN], "Camera", "UnityEngine", "Camera.get_main");
    next.enemyOff = field_off(pid, hits[N_ENEMY], kBase, 8, "EnemyPawns");
    next.aliveOff = field_off(pid, hits[N_ALIVE], kPawn, 1, "m_IsAlive");
    next.botOff = field_off(pid, hits[N_BOT], kPawn, 1, "m_IsBot");
    next.infoOff = field_off(pid, hits[N_INFO], kTarget, 8, "m_AttackableInfo");
    next.hpOff = field_off(pid, hits[N_HP], kInfo, 4, "m_Health");
    next.maxOff = field_off(pid, hits[N_MAX], kInfo, 4, "m_MaxHealth");
    next.posOff = field_off(pid, hits[N_LAST_F], kPawn, 4, "m_LastPawnPos");
    if (next.posOff < 0) next.posOff = method_vec(pid, ranges, hits[N_LAST_M], "Pawn");
    next.headOff = field_off(pid, hits[N_HEAD_F], kPawn, 4, "m_HeadPosition");
    if (next.headOff < 0) next.headOff = method_vec(pid, ranges, hits[N_HEAD_M], "Pawn");
    next.nameOff = method_ptr_off(pid, ranges, hits[N_NAME_M], "Pawn");
    if (!next.matchSlot || !next.camSlot || next.enemyOff < 0 || next.posOff < 0) {
        const uintptr_t base = unity_file_base(pid);
        const uintptr_t matchFn = base + 0x801191C;
        const uintptr_t localFn = base + 0x8012814;
        const uintptr_t camFn = base + 0x52D5CDC;
        const uintptr_t posFn = base + 0x538AABC;
        const uintptr_t headFn = base + 0x538A478;
        if (base && in_exec(ranges, matchFn)) {
            elog("[esp] libunity base=0x%llx using dump addresses", (unsigned long long)base);
            auto slot_from = [&](uintptr_t fn, const char *label) -> uintptr_t {
                uintptr_t slot = 0;
                const uintptr_t body = follow_b(pid, fn);
                if (!decode_static_slot(pid, body, slot)) {
                    log_code(pid, label, body);
                    return 0;
                }
                uintptr_t probe = 0;
                mem_read(pid, slot, &probe, sizeof(probe));
                if (probe && !ptr_ok(probe)) return 0;
                elog("[esp] %s slot=0x%llx", label, (unsigned long long)slot);
                return slot;
            };
            if (!next.matchSlot) next.matchSlot = slot_from(matchFn, "get_MatchGame");
            if (!next.localSlot) next.localSlot = slot_from(localFn, "get_LocalPawn");
            if (!next.camSlot) next.camSlot = slot_from(camFn, "Camera.get_main");
            if (next.posOff < 0) next.posOff = decode_vec3(pid, follow_b(pid, posFn));
            if (next.headOff < 0) next.headOff = decode_vec3(pid, follow_b(pid, headFn));
            if (next.enemyOff < 0) next.enemyOff = 0x178;
            if (next.aliveOff < 0) next.aliveOff = 0x550;
            if (next.botOff < 0) next.botOff = 0x5C1;
            if (next.infoOff < 0) next.infoOff = 0x78;
            if (next.hpOff < 0) next.hpOff = 0x34;
            if (next.maxOff < 0) next.maxOff = 0x38;
        } else {
            elog("[esp] libunity base missing for dump addresses");
        }
    }
    next.attempts = (next.matchSlot || !hits[N_MATCH].empty()) ? 3 : attempt;
    next.ready = next.matchSlot && next.camSlot && next.enemyOff > 0 && next.posOff > 0;
    if (next.ready)
        std::snprintf(next.status, sizeof(next.status), "ESP ready");
    else
        std::snprintf(next.status, sizeof(next.status), "ESP fields missing");
    elog("[esp] %s match=%d cam=%d enemy=%#x pos=%#x head=%#x", next.ready ? "ready" : "not ready",
         next.matchSlot ? 1 : 0, next.camSlot ? 1 : 0, next.enemyOff, next.posOff, next.headOff);

    std::lock_guard<std::mutex> lock(g_mu);
    if (g_snap.generation != gen || g_snap.pid != pid) return;
    const int tries = g_snap.matrixTries;
    const int miss = g_snap.matrixMiss;
    const int view = g_snap.viewOff;
    const int proj = g_snap.projOff;
    const int order = g_snap.mulOrder;
    const int baseField = g_snap.baseField;
    const int baseNative = g_snap.baseIsNative;
    g_snap = next;
    g_snap.generation = gen;
    g_snap.matrixTries = tries;
    g_snap.matrixMiss = miss;
    g_snap.viewOff = view;
    g_snap.projOff = proj;
    g_snap.mulOrder = order;
    g_snap.baseField = baseField;
    g_snap.baseIsNative = baseNative;
}

void codm_esp_draw(ImDrawList *draw, int width, int height, bool enable, bool line, bool box, bool name,
                   bool health, bool distance, bool counter) {
    if (!draw || width < 100 || height < 100) return;
    if (!enable && !counter) {
        if ((line || box || name || health || distance) && draw && width > 100)
            paint_status(draw, width, "Turn on ESP ENABLED");
        return;
    }
    if (enable && !line && !box && !name && !health && !distance) {
        line = true;
        box = true;
    }

    Snap snap;
    {
        std::lock_guard<std::mutex> lock(g_mu);
        snap = g_snap;
    }
    if (!snap.ready || snap.pid <= 0) {
        paint_status(draw, width, snap.status[0] ? snap.status : "ESP resolving");
        return;
    }

    uintptr_t match = 0, local = 0, cam = 0;
    if (!mem_read(snap.pid, snap.matchSlot, &match, sizeof(match)) || !ptr_ok(match)) {
        paint_status(draw, width, "ESP not in match");
        return;
    }
    if (snap.localSlot) mem_read(snap.pid, snap.localSlot, &local, sizeof(local));
    if (!ptr_ok(local)) local = 0;
    if (!mem_read(snap.pid, snap.camSlot, &cam, sizeof(cam)) || !ptr_ok(cam)) {
        paint_status(draw, width, "ESP no camera");
        return;
    }

    uintptr_t list = 0;
    if (!mem_read(snap.pid, match + (uintptr_t)snap.enemyOff, &list, sizeof(list)) || !ptr_ok(list)) {
        paint_status(draw, width, "ESP no enemies");
        return;
    }
    int32_t size = 0;
    uintptr_t items = 0;
    mem_read(snap.pid, list + 0x18, &size, sizeof(size));
    mem_read(snap.pid, list + 0x10, &items, sizeof(items));
    if (!ptr_ok(items) || size < 0 || size > 160) {
        char msg[48];
        std::snprintf(msg, sizeof(msg), "ESP list %d", (int)size);
        paint_status(draw, width, msg);
        return;
    }
    if (size > 100) size = 100;
    if (size == 0) {
        paint_status(draw, width, "ESP 0");
        return;
    }
    int32_t maxl = 0;
    if (mem_read(snap.pid, items + 0x18, &maxl, sizeof(maxl)) && maxl > 0 && maxl < size) size = maxl;

    uintptr_t pawns[100];
    if (!mem_read(snap.pid, items + 0x20, pawns, (size_t)size * sizeof(uintptr_t))) {
        paint_status(draw, width, "ESP no enemies");
        return;
    }

    struct Body {
        float x, y, z, hx, hy, hz;
        int hp, maxhp;
        bool bot;
        char title[40];
    };
    Body bodies[100];
    int nb = 0;
    float sampleX = 0, sampleY = 0, sampleZ = 0;
    bool haveSample = false;
    for (int i = 0; i < size && nb < 100; ++i) {
        uintptr_t pawn = pawns[i];
        if (!ptr_ok(pawn) || pawn == local) continue;
        if (snap.aliveOff > 0) {
            uint8_t alive = 0;
            if (!mem_read(snap.pid, pawn + (uintptr_t)snap.aliveOff, &alive, 1) || !alive) continue;
        }
        float pos[3] = {};
        if (!mem_read(snap.pid, pawn + (uintptr_t)snap.posOff, pos, sizeof(pos)) || !finite3(pos[0], pos[1], pos[2]))
            continue;
        Body b{};
        b.x = pos[0];
        b.y = pos[1];
        b.z = pos[2];
        b.hx = pos[0];
        b.hy = pos[1] + 1.65f;
        b.hz = pos[2];
        if (snap.headOff > 0) {
            float head[3] = {};
            if (mem_read(snap.pid, pawn + (uintptr_t)snap.headOff, head, sizeof(head)) &&
                finite3(head[0], head[1], head[2])) {
                b.hx = head[0];
                b.hy = head[1];
                b.hz = head[2];
            }
        }
        if (snap.botOff > 0) {
            uint8_t bot = 0;
            mem_read(snap.pid, pawn + (uintptr_t)snap.botOff, &bot, 1);
            b.bot = bot != 0;
        }
        if (health && snap.infoOff > 0 && snap.hpOff > 0 && snap.maxOff > 0) {
            uintptr_t info = 0;
            if (mem_read(snap.pid, pawn + (uintptr_t)snap.infoOff, &info, sizeof(info)) && ptr_ok(info)) {
                int32_t hp = 0, mx = 0;
                mem_read(snap.pid, info + (uintptr_t)snap.hpOff, &hp, sizeof(hp));
                mem_read(snap.pid, info + (uintptr_t)snap.maxOff, &mx, sizeof(mx));
                if (hp > 0 && mx > 0 && hp <= 10000 && mx <= 10000) {
                    b.hp = hp;
                    b.maxhp = mx;
                }
            }
        }
        if (name && snap.nameOff > 0) {
            uintptr_t str = 0;
            int32_t len = 0;
            if (mem_read(snap.pid, pawn + (uintptr_t)snap.nameOff, &str, sizeof(str)) && ptr_ok(str) &&
                mem_read(snap.pid, str + 0x10, &len, sizeof(len)) && len > 0 && len <= 24) {
                uint16_t u[24] = {};
                if (mem_read(snap.pid, str + 0x14, u, (size_t)len * 2)) {
                    int o = 0;
                    for (int c = 0; c < len && o < 36; ++c) {
                        unsigned ch = u[c];
                        if (ch < 32 || ch > 126) ch = '?';
                        b.title[o++] = (char)ch;
                    }
                    b.title[o] = 0;
                }
            }
        }
        if (!haveSample) {
            sampleX = b.x;
            sampleY = b.y;
            sampleZ = b.z;
            haveSample = true;
        }
        bodies[nb++] = b;
    }

    auto load_vp = [&](float *vp) -> bool {
        uintptr_t base = cam;
        if (snap.baseIsNative) {
            if (!mem_read(snap.pid, cam + (uintptr_t)snap.baseField, &base, sizeof(base)) || !ptr_ok(base))
                return false;
        }
        float view[16], proj[16];
        if (!mem_read(snap.pid, base + (uintptr_t)snap.viewOff, view, sizeof(view))) return false;
        if (!mem_read(snap.pid, base + (uintptr_t)snap.projOff, proj, sizeof(proj))) return false;
        if (!is_proj(proj)) return false;
        if (snap.mulOrder == 0) mul4(proj, view, vp);
        else mul4(view, proj, vp);
        return true;
    };

    if (snap.viewOff < 0 && haveSample && snap.matrixTries < 12) {
        int viewOff = -1, projOff = -1, order = 0, baseField = 0x10, baseNative = 1;
        bool got = false;
        const int fields[] = {0x10, 0x18, 0x20};
        for (int field : fields) {
            uintptr_t native = 0;
            if (!mem_read(snap.pid, cam + (uintptr_t)field, &native, sizeof(native)) || !ptr_ok(native))
                continue;
            uint8_t buf[0x900];
            if (!mem_read(snap.pid, native, buf, sizeof(buf))) continue;
            if (find_matrices(buf, sizeof(buf), sampleX, sampleY, sampleZ, width, height, viewOff, projOff,
                              order)) {
                baseField = field;
                baseNative = 1;
                got = true;
                break;
            }
        }
        if (!got) {
            uint8_t buf[0x300];
            if (mem_read(snap.pid, cam, buf, sizeof(buf)) &&
                find_matrices(buf, sizeof(buf), sampleX, sampleY, sampleZ, width, height, viewOff, projOff,
                              order)) {
                baseNative = 0;
                got = true;
            }
        }
        std::lock_guard<std::mutex> lock(g_mu);
        if (g_snap.generation == snap.generation && g_snap.pid == snap.pid) {
            g_snap.matrixTries++;
            if (got) {
                g_snap.viewOff = viewOff;
                g_snap.projOff = projOff;
                g_snap.mulOrder = order;
                g_snap.baseField = baseField;
                g_snap.baseIsNative = baseNative;
                g_snap.matrixMiss = 0;
                std::snprintf(g_snap.status, sizeof(g_snap.status), "ESP live");
                elog("[esp] camera matrix view=%#x proj=%#x", viewOff, projOff);
            }
        }
        if (got) {
            snap.viewOff = viewOff;
            snap.projOff = projOff;
            snap.mulOrder = order;
            snap.baseField = baseField;
            snap.baseIsNative = baseNative;
        }
    }

    float vp[16];
    bool haveVp = snap.viewOff >= 0 && load_vp(vp);
    if (!haveVp) {
        char msg[64];
        std::snprintf(msg, sizeof(msg), "ESP waiting camera  %d", nb);
        paint_status(draw, width, msg);
        return;
    }

    int drawn = 0;
    for (int i = 0; i < nb; ++i) {
        const Body &b = bodies[i];
        float fx, fy, hx, hy;
        if (!project(vp, b.x, b.y, b.z, width, height, fx, fy)) continue;
        bool haveHead = project(vp, b.hx, b.hy, b.hz, width, height, hx, hy);
        if (!haveHead) {
            hx = fx;
            hy = fy - 48.f;
        }
        if (hy > fy) std::swap(hy, fy);
        const float bh = std::max(8.f, fy - hy);
        const float bw = bh * 0.45f;
        ImU32 col = b.bot ? IM_COL32(190, 190, 190, 230) : IM_COL32(255, 255, 255, 230);
        if (line) draw->AddLine(ImVec2(width * 0.5f, (float)height - 4.f), ImVec2(fx, fy), col, 1.5f);
        if (box) draw->AddRect(ImVec2(fx - bw * 0.5f, hy), ImVec2(fx + bw * 0.5f, hy + bh), col, 0.f, 0, 1.4f);
        if (name && b.title[0]) {
            ImVec2 sz = ImGui::CalcTextSize(b.title);
            draw->AddText(ImVec2(fx - sz.x * 0.5f, hy - sz.y - 2.f), IM_COL32(255, 255, 255, 240), b.title);
        }
        if (distance && local) {
            float lp[3] = {};
            if (snap.posOff > 0 && mem_read(snap.pid, local + (uintptr_t)snap.posOff, lp, sizeof(lp)) &&
                finite3(lp[0], lp[1], lp[2])) {
                float dx = lp[0] - b.x, dy = lp[1] - b.y, dz = lp[2] - b.z;
                int meters = (int)std::sqrt(dx * dx + dy * dy + dz * dz);
                char lab[16];
                std::snprintf(lab, sizeof(lab), "%dm", meters);
                draw->AddText(ImVec2(fx + 8.f, fy), IM_COL32(255, 255, 255, 230), lab);
            }
        }
        if (health && b.maxhp > 0) {
            float t = (float)b.hp / (float)b.maxhp;
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            float x = fx - bw * 0.5f - 6.f;
            draw->AddRectFilled(ImVec2(x, hy), ImVec2(x + 3.f, hy + bh), IM_COL32(40, 40, 40, 200));
            draw->AddRectFilled(ImVec2(x, hy + bh * (1.f - t)), ImVec2(x + 3.f, hy + bh),
                                 t > 0.5f ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 60, 60, 255));
        }
        ++drawn;
    }
    char top[48];
    std::snprintf(top, sizeof(top), "ESP %d", drawn);
    paint_status(draw, width, top);
}
