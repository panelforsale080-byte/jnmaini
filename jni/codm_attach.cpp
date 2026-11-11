#include "codm_attach.h"
#include "codm_esp.h"

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <elf.h>
#include <errno.h>
#include <mutex>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr const char *kPkg = "com.garena.game.codm";
constexpr size_t kMaxLines = 80;
constexpr size_t kLineCap = 240;

std::mutex g_mu;
char g_lines[kMaxLines][kLineCap];
size_t g_start = 0;
size_t g_count = 0;
char g_status[192] = "Waiting for CODM";
char g_lastLog[kLineCap] = {};
std::atomic<int> g_state{0};

pid_t g_pid = 0;
uintptr_t g_anogs = 0;
uintptr_t g_unity = 0;
uintptr_t g_il2 = 0;
uintptr_t g_domainGet = 0;
uintptr_t g_domain = 0;
char g_libName[64] = {};
bool g_scanned = false;
int g_seenMods = -1;
bool g_unityIl2 = false;
pid_t g_hintPid = 0;
char g_hintCmd[180] = {};

void push_line(const char *line) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!line) line = "";
    for (size_t i = 0; i < g_count; ++i) {
        if (std::strcmp(g_lines[(g_start + i) % kMaxLines], line) == 0) return;
    }
    std::snprintf(g_lastLog, sizeof(g_lastLog), "%s", line);
    size_t slot;
    if (g_count < kMaxLines) {
        slot = (g_start + g_count) % kMaxLines;
        g_count++;
    } else {
        slot = g_start;
        g_start = (g_start + 1) % kMaxLines;
    }
    std::snprintf(g_lines[slot], kLineCap, "%s", line ? line : "");
}

void logf(const char *fmt, ...) {
    char buf[kLineCap];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    push_line(buf);
}

void set_status(const char *fmt, ...) {
    char buf[sizeof(g_status)];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    std::lock_guard<std::mutex> lock(g_mu);
    std::snprintf(g_status, sizeof(g_status), "%s", buf);
}

bool mem_read(pid_t pid, uintptr_t addr, void *buf, size_t n) {
    if (pid <= 0 || !addr || !buf || !n) return false;
    iovec local{buf, n};
    iovec remote{reinterpret_cast<void *>(addr), n};
    return syscall(SYS_process_vm_readv, pid, &local, 1, &remote, 1, 0) == (ssize_t)n;
}

bool mem_ptr_ok(uintptr_t p) {
    return p >= 0x10000ULL && p < 0x800000000000ULL && ((p & 7ULL) == 0);
}

bool read_cmdline(pid_t pid, char *out, size_t cap) {
    if (!out || !cap) return false;
    out[0] = 0;
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/cmdline", (int)pid);
    FILE *fp = std::fopen(path, "rb");
    if (!fp) return false;
    size_t n = std::fread(out, 1, cap - 1, fp);
    std::fclose(fp);
    out[n] = 0;
    for (size_t i = 0; i < n; ++i)
        if (out[i] == '\0') out[i] = ' ';
    return n > 0;
}

uintptr_t map_base(pid_t pid, const char *lib, char *pathOut, size_t pathCap) {
    if (pathOut && pathCap) pathOut[0] = 0;
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return 0;
    char line[768];
    uintptr_t base = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        if (!std::strstr(line, lib)) continue;
        base = (uintptr_t)std::strtoull(line, nullptr, 16);
        if (pathOut && pathCap) {
            const char *slash = std::strrchr(line, '/');
            const char *src = slash ? slash : line;
            while (*src == ' ') ++src;
            std::snprintf(pathOut, pathCap, "%s", src);
            size_t len = std::strlen(pathOut);
            while (len && (pathOut[len - 1] == '\n' || pathOut[len - 1] == '\r' || pathOut[len - 1] == ' '))
                pathOut[--len] = 0;
            if (slash) {
                char full[512];
                const char *p = std::strchr(line, '/');
                if (p) {
                    std::snprintf(full, sizeof(full), "%s", p);
                    size_t fl = std::strlen(full);
                    while (fl && (full[fl - 1] == '\n' || full[fl - 1] == '\r' || full[fl - 1] == ' '))
                        full[--fl] = 0;
                    std::snprintf(pathOut, pathCap, "%s", full);
                }
            }
        }
        break;
    }
    std::fclose(fp);
    return base;
}

bool maps_contain(pid_t pid, const char *needle) {
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;
    char line[768];
    bool hit = false;
    while (std::fgets(line, sizeof(line), fp)) {
        if (std::strstr(line, needle)) {
            hit = true;
            break;
        }
    }
    std::fclose(fp);
    return hit;
}

struct Mod {
    char path[512];
    uintptr_t base;
    int score;
};

int mod_score(const char *path) {
    const char *base = std::strrchr(path, '/');
    base = base ? base + 1 : path;
    if (std::strstr(base, "il2cpp") || std::strstr(base, "Il2Cpp")) return 100;
    if (std::strstr(base, "GameAssembly") || std::strstr(base, "GameCore") || std::strstr(base, "codm"))
        return 90;
    if (std::strstr(base, "unity") || std::strstr(base, "Unity")) return 40;
    return 20;
}

bool collect_mods(pid_t pid, std::vector<Mod> &out) {
    out.clear();
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;
    char line[768];
    while (std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0, off = 0;
        char perms[8] = {};
        char file[512] = {};
        if (std::sscanf(line, "%lx-%lx %7s %lx %*s %*s %511[^\n]", &start, &end, perms, &off, file) < 5)
            continue;
        if (off != 0 || perms[0] != 'r' || file[0] != '/') continue;
        if (!std::strstr(file, ".so")) continue;
        if (std::strstr(file, "/system/") || std::strstr(file, "/apex/") || std::strstr(file, "/vendor/"))
            continue;
        bool seen = false;
        for (const Mod &m : out) {
            if (std::strcmp(m.path, file) == 0) {
                seen = true;
                break;
            }
        }
        if (seen || out.size() >= 96) continue;
        Mod m{};
        std::snprintf(m.path, sizeof(m.path), "%s", file);
        m.base = (uintptr_t)start;
        m.score = mod_score(file);
        out.push_back(m);
    }
    std::fclose(fp);
    for (size_t i = 1; i < out.size(); ++i) {
        Mod key = out[i];
        size_t j = i;
        while (j > 0 && out[j - 1].score < key.score) {
            out[j] = out[j - 1];
            --j;
        }
        out[j] = key;
    }
    return !out.empty();
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

bool domain_from_getter(pid_t pid, uintptr_t func, uintptr_t &domainOut) {
    domainOut = 0;
    if (!func) return false;
    uint32_t ins[32] = {};
    if (!mem_read(pid, func, ins, sizeof(ins))) return false;
    uint64_t x[31] = {};
    for (int i = 0; i < 28; ++i) {
        const uint32_t w = ins[i];
        const uint32_t w2 = ins[i + 1];
        const uint64_t pc = func + (uint64_t)i * 4;
        int64_t page = 0;
        if (decode_adrp(w, pc, page)) {
            const int rd = (int)(w & 0x1fu);
            if (rd >= 0 && rd < 31) x[rd] = (uint64_t)page;
        }
        if ((w & 0xFFC00000u) == 0x91000000u) {
            const int rd = (int)(w & 0x1fu);
            const int rn = (int)((w >> 5) & 0x1fu);
            const uint64_t imm = (uint64_t)((w >> 10) & 0xFFFu);
            if (rd < 31 && rn < 31) x[rd] = x[rn] + imm;
        }
        if ((w2 & 0xFFC00000u) == 0xF9400000u) {
            const int rd = (int)(w2 & 0x1fu);
            const int rn = (int)((w2 >> 5) & 0x1fu);
            const uint64_t off = (uint64_t)(((w2 >> 10) & 0xFFFu) * 8);
            if (rd < 31 && rn < 31) {
                uintptr_t ptr = 0;
                if (mem_read(pid, (uintptr_t)(x[rn] + off), &ptr, sizeof(ptr)) && mem_ptr_ok(ptr)) {
                    if (rd == 0) domainOut = ptr;
                    x[rd] = ptr;
                }
            }
        }
        if ((w & 0xFFFFFC1Fu) == 0xD65F0000u) {
            if (domainOut) return true;
            if (mem_ptr_ok(x[0])) {
                domainOut = x[0];
                return true;
            }
        }
    }
    return domainOut != 0;
}

bool lookup_syms(const char *sym, const char *strData, size_t strSize, const Elf64_Sym *syms, size_t n,
                 uintptr_t bias, uintptr_t &addr) {
    for (size_t i = 0; i < n; ++i) {
        const Elf64_Sym &s = syms[i];
        if (!s.st_name || s.st_name >= strSize || !s.st_value) continue;
        const int type = ELF64_ST_TYPE(s.st_info);
        if (type != STT_FUNC && type != STT_OBJECT && type != STT_NOTYPE) continue;
        const char *have = strData + s.st_name;
        const size_t nlen = std::strlen(sym);
        if (std::strncmp(have, sym, nlen) != 0) continue;
        if (have[nlen] != '\0' && have[nlen] != '@') continue;
        addr = bias + (uintptr_t)s.st_value;
        return true;
    }
    return false;
}

bool read_remote_span(pid_t pid, uintptr_t addr, void *buf, size_t n) {
    auto *dst = static_cast<uint8_t *>(buf);
    size_t off = 0;
    while (off < n) {
        size_t chunk = n - off;
        if (chunk > 64 * 1024) chunk = 64 * 1024;
        if (!mem_read(pid, addr + off, dst + off, chunk)) return false;
        off += chunk;
    }
    return true;
}

bool sym_from_dynamic(pid_t pid, uintptr_t base, const char *sym, uintptr_t &addr) {
    addr = 0;
    Elf64_Ehdr eh{};
    if (!mem_read(pid, base, &eh, sizeof(eh))) return false;
    if (eh.e_ident[EI_MAG0] != 0x7f || eh.e_ident[EI_MAG1] != 'E' || eh.e_ident[EI_CLASS] != ELFCLASS64)
        return false;
    if (eh.e_phnum == 0 || eh.e_phnum > 64 || !eh.e_phoff) return false;
    Elf64_Phdr ph[64];
    if (!mem_read(pid, base + eh.e_phoff, ph, eh.e_phnum * sizeof(Elf64_Phdr))) return false;
    uintptr_t minV = UINTPTR_MAX;
    const Elf64_Phdr *dynPh = nullptr;
    for (int i = 0; i < eh.e_phnum; ++i) {
        if (ph[i].p_type == PT_LOAD && ph[i].p_vaddr < minV) minV = (uintptr_t)ph[i].p_vaddr;
        if (ph[i].p_type == PT_DYNAMIC) dynPh = &ph[i];
    }
    if (!dynPh || minV == UINTPTR_MAX) return false;
    const uintptr_t bias = base - minV;
    uintptr_t dynAddr = bias + (uintptr_t)dynPh->p_vaddr;
    size_t dynBytes = (size_t)dynPh->p_memsz;
    if (dynBytes < sizeof(Elf64_Dyn) || dynBytes > 8192) dynBytes = 4096;

    uintptr_t symtab = 0, strtab = 0, hash = 0, gnu = 0;
    size_t strsz = 0;
    for (size_t off = 0; off + sizeof(Elf64_Dyn) <= dynBytes; off += sizeof(Elf64_Dyn)) {
        Elf64_Dyn d{};
        if (!mem_read(pid, dynAddr + off, &d, sizeof(d))) return false;
        if (d.d_tag == DT_NULL) break;
        if (d.d_tag == DT_SYMTAB) symtab = (uintptr_t)d.d_un.d_ptr;
        else if (d.d_tag == DT_STRTAB) strtab = (uintptr_t)d.d_un.d_ptr;
        else if (d.d_tag == DT_STRSZ) strsz = (size_t)d.d_un.d_val;
        else if (d.d_tag == DT_HASH) hash = (uintptr_t)d.d_un.d_ptr;
        else if (d.d_tag == DT_GNU_HASH) gnu = (uintptr_t)d.d_un.d_ptr;
    }
    auto fix = [&](uintptr_t p) -> uintptr_t {
        if (!p) return 0;
        if (p < bias) return bias + p;
        return p;
    };
    symtab = fix(symtab);
    strtab = fix(strtab);
    hash = fix(hash);
    gnu = fix(gnu);
    if (!symtab || !strtab || strsz == 0 || strsz > 32u * 1024u * 1024u) return false;

    size_t count = 0;
    if (hash) {
        uint32_t hdr[2] = {};
        if (mem_read(pid, hash, hdr, sizeof(hdr)) && hdr[1] > 0 && hdr[1] < 500000) count = hdr[1];
    }
    if (!count && gnu) {
        uint32_t gh[4] = {};
        if (!mem_read(pid, gnu, gh, sizeof(gh))) return false;
        const uint32_t nbuckets = gh[0];
        const uint32_t symoff = gh[1];
        const uint32_t bloomSize = gh[2];
        if (!nbuckets || nbuckets > 8192 || bloomSize > 8192) return false;
        const uintptr_t buckets = gnu + sizeof(gh) + bloomSize * sizeof(uint64_t);
        uint32_t maxIdx = symoff;
        int steps = 0;
        const size_t chunk = 1024;
        for (uint32_t b = 0; b < nbuckets; b += chunk) {
            uint32_t n = nbuckets - b;
            if (n > chunk) n = chunk;
            uint32_t vals[1024];
            if (!mem_read(pid, buckets + b * sizeof(uint32_t), vals, n * sizeof(uint32_t))) return false;
            for (uint32_t i = 0; i < n; ++i) {
                uint32_t idx = vals[i];
                if (!idx) continue;
                for (int step = 0; step < 64; ++step) {
                    if (++steps > 20000) return false;
                    if (idx > maxIdx) maxIdx = idx;
                    uint32_t chain = 0;
                    if (idx < symoff) break;
                    if (!mem_read(pid, buckets + nbuckets * sizeof(uint32_t) + (idx - symoff) * sizeof(uint32_t),
                                  &chain, sizeof(chain)))
                        break;
                    if (chain & 1u) break;
                    ++idx;
                }
            }
        }
        count = (size_t)maxIdx + 1;
    }
    if (!count || count > 500000) return false;

    std::vector<char> strings(strsz);
    if (!read_remote_span(pid, strtab, strings.data(), strsz)) return false;
    const size_t step = 256;
    for (size_t i = 0; i < count; i += step) {
        size_t n = count - i;
        if (n > step) n = step;
        std::vector<Elf64_Sym> syms(n);
        if (!read_remote_span(pid, symtab + i * sizeof(Elf64_Sym), syms.data(), n * sizeof(Elf64_Sym)))
            return false;
        if (lookup_syms(sym, strings.data(), strings.size(), syms.data(), n, bias, addr)) return true;
    }
    return false;
}

bool sym_from_file(const char *path, uintptr_t mapBase, const char *sym, uintptr_t &addr) {
    addr = 0;
    FILE *f = std::fopen(path, "rb");
    if (!f) return false;
    Elf64_Ehdr eh{};
    if (std::fread(&eh, 1, sizeof(eh), f) != sizeof(eh) || eh.e_ident[EI_MAG0] != 0x7f) {
        std::fclose(f);
        return false;
    }
    if (eh.e_phnum == 0 || eh.e_phnum > 64) {
        std::fclose(f);
        return false;
    }
    if (std::fseek(f, (long)eh.e_phoff, SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    Elf64_Phdr ph[64];
    if (std::fread(ph, sizeof(Elf64_Phdr), eh.e_phnum, f) != eh.e_phnum) {
        std::fclose(f);
        return false;
    }
    uintptr_t minV = UINTPTR_MAX;
    for (int i = 0; i < eh.e_phnum; ++i)
        if (ph[i].p_type == PT_LOAD && ph[i].p_vaddr < minV) minV = (uintptr_t)ph[i].p_vaddr;
    if (minV == UINTPTR_MAX || eh.e_shnum == 0 || eh.e_shnum > 4096 || !eh.e_shoff) {
        std::fclose(f);
        return false;
    }
    const uintptr_t bias = mapBase - minV;
    if (std::fseek(f, (long)eh.e_shoff, SEEK_SET) != 0) {
        std::fclose(f);
        return false;
    }
    std::vector<Elf64_Shdr> sh(eh.e_shnum);
    if (std::fread(sh.data(), sizeof(Elf64_Shdr), sh.size(), f) != sh.size() || eh.e_shstrndx >= eh.e_shnum) {
        std::fclose(f);
        return false;
    }
    const Elf64_Shdr &shstr = sh[eh.e_shstrndx];
    if (!shstr.sh_size || shstr.sh_size > 512 * 1024) {
        std::fclose(f);
        return false;
    }
    std::vector<char> names(shstr.sh_size);
    if (std::fseek(f, (long)shstr.sh_offset, SEEK_SET) != 0 ||
        std::fread(names.data(), 1, names.size(), f) != names.size()) {
        std::fclose(f);
        return false;
    }
    const Elf64_Shdr *dynsym = nullptr;
    const Elf64_Shdr *dynstr = nullptr;
    const Elf64_Shdr *symtab = nullptr;
    const Elf64_Shdr *strtab = nullptr;
    for (const Elf64_Shdr &s : sh) {
        if (s.sh_name >= names.size()) continue;
        const char *nm = names.data() + s.sh_name;
        if (std::strcmp(nm, ".dynsym") == 0) dynsym = &s;
        else if (std::strcmp(nm, ".dynstr") == 0) dynstr = &s;
        else if (std::strcmp(nm, ".symtab") == 0) symtab = &s;
        else if (std::strcmp(nm, ".strtab") == 0) strtab = &s;
    }
    auto try_table = [&](const Elf64_Shdr *symsh, const Elf64_Shdr *strsh) -> bool {
        if (!symsh || !strsh || symsh->sh_entsize != sizeof(Elf64_Sym)) return false;
        if (!strsh->sh_size || strsh->sh_size > 48u * 1024u * 1024u) return false;
        if (!symsh->sh_size || symsh->sh_size > 48u * 1024u * 1024u) return false;
        std::vector<char> str(strsh->sh_size);
        std::vector<Elf64_Sym> syms(symsh->sh_size / sizeof(Elf64_Sym));
        if (std::fseek(f, (long)strsh->sh_offset, SEEK_SET) != 0) return false;
        if (std::fread(str.data(), 1, str.size(), f) != str.size()) return false;
        if (std::fseek(f, (long)symsh->sh_offset, SEEK_SET) != 0) return false;
        if (std::fread(syms.data(), sizeof(Elf64_Sym), syms.size(), f) != syms.size()) return false;
        return lookup_syms(sym, str.data(), str.size(), syms.data(), syms.size(), bias, addr);
    };
    const bool ok = try_table(dynsym, dynstr) || try_table(symtab, strtab);
    std::fclose(f);
    return ok;
}

static bool file_has_string(const char *path, const char *needle) {
    FILE *f = std::fopen(path, "rb");
    if (!f || !needle || !needle[0]) {
        if (f) std::fclose(f);
        return false;
    }
    const size_t nlen = std::strlen(needle);
    char buf[4096 + 80];
    size_t keep = 0;
    bool found = false;
    while (!found) {
        const size_t n = std::fread(buf + keep, 1, 4096, f);
        if (!n) break;
        const size_t total = keep + n;
        for (size_t i = 0; i + nlen <= total; ++i) {
            if (std::memcmp(buf + i, needle, nlen) == 0) {
                found = true;
                break;
            }
        }
        keep = nlen > 1 && total >= nlen ? nlen - 1 : total;
        if (keep) std::memmove(buf, buf + total - keep, keep);
    }
    std::fclose(f);
    return found;
}

bool scan_rx_elfs(pid_t pid, uintptr_t &addr, char *name, size_t nameCap) {
    addr = 0;
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return false;
    char line[768];
    int elfs = 0;
    int checked = 0;
    while (std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0, off = 0;
        char perms[8] = {};
        char file[512] = {};
        const int n = std::sscanf(line, "%lx-%lx %7s %lx %*s %*s %511[^\n]", &start, &end, perms, &off, file);
        if (n < 4 || perms[0] != 'r' || off != 0) continue;
        if (n >= 5 && std::strstr(file, ".so")) continue;
        uint32_t mag = 0;
        if (!mem_read(pid, (uintptr_t)start, &mag, sizeof(mag)) || mag != 0x464c457f) continue;
        ++elfs;
        uintptr_t sym = 0;
        if (sym_from_dynamic(pid, (uintptr_t)start, "il2cpp_domain_get", sym) && sym) {
            const char *tag = (n >= 5 && file[0]) ? file : "anon";
            const char *base = std::strrchr(tag, '/');
            base = base ? base + 1 : tag;
            std::snprintf(name, nameCap, "%s", base);
            addr = sym;
            logf("[il2cpp] unpacked %s base=0x%lx domain_get=0x%llx", base, start,
                 (unsigned long long)sym);
            std::fclose(fp);
            return true;
        }
        if (++checked >= 48) break;
    }
    std::fclose(fp);
    if (elfs) logf("[il2cpp] unpacked elf regions=%d, no domain_get", elfs);
    return false;
}

void log_disk_siblings(const std::vector<Mod> &mods) {
    const char *unity = nullptr;
    for (const Mod &m : mods) {
        if (std::strstr(m.path, "libunity.so")) {
            unity = m.path;
            break;
        }
    }
    if (!unity) return;
    char dir[512];
    std::snprintf(dir, sizeof(dir), "%s", unity);
    char *slash = std::strrchr(dir, '/');
    if (!slash) return;
    *slash = 0;
    DIR *d = opendir(dir);
    if (!d) {
        logf("[disk] cannot open %s", dir);
        return;
    }
    int listed = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != nullptr) {
        const char *nm = ent->d_name;
        bool interesting = false;
        for (const char *s = nm; *s && !interesting; ++s) {
            const char *a = s;
            const char *b = "il2cpp";
            while (*a && *b && ((*a | 32) == (*b | 32))) {
                ++a;
                ++b;
            }
            if (!*b) interesting = true;
        }
        if (!interesting && !std::strstr(nm, "logic")) continue;
        logf("[disk] %s/%s", dir, nm);
        if (++listed >= 8) break;
    }
    if (!listed) logf("[disk] no libil2cpp next to libunity");
    closedir(d);
}

bool find_domain_get(pid_t pid, const std::vector<Mod> &mods, uintptr_t &addr, char *name, size_t nameCap,
                     bool verbose) {
    addr = 0;
    char listed[240] = "[mods]";
    size_t used = std::strlen(listed);
    if (!verbose) {
        listed[0] = 0;
        used = 0;
    }
    for (const Mod &m : mods) {
        if (!verbose) break;
        const char *base = std::strrchr(m.path, '/');
        base = base ? base + 1 : m.path;
        const size_t n = std::strlen(base);
        if (used + n + 2 >= sizeof(listed)) {
            logf("%s", listed);
            used = 0;
            listed[0] = 0;
        }
        listed[used++] = ' ';
        std::memcpy(listed + used, base, n);
        used += n;
        listed[used] = 0;
    }
    if (listed[0]) logf("%s", listed);

    for (const Mod &m : mods) {
        const char *base = std::strrchr(m.path, '/');
        base = base ? base + 1 : m.path;
        if (sym_from_file(m.path, m.base, "il2cpp_domain_get", addr) ||
            sym_from_dynamic(pid, m.base, "il2cpp_domain_get", addr)) {
            std::snprintf(name, nameCap, "%s", base);
            logf("[il2cpp] %s base=0x%llx domain_get=0x%llx", base,
                 (unsigned long long)m.base, (unsigned long long)addr);
            return true;
        }
    }

    if (!verbose) return false;
    char hits[240] = {};
    size_t hused = 0;
    for (const Mod &m : mods) {
        const char *base = std::strrchr(m.path, '/');
        base = base ? base + 1 : m.path;
        if (!file_has_string(m.path, "il2cpp_domain_get")) continue;
        const size_t n = std::strlen(base);
        if (hused + n + 2 >= sizeof(hits)) break;
        if (hused) hits[hused++] = ' ';
        std::memcpy(hits + hused, base, n);
        hused += n;
        hits[hused] = 0;
    }
    if (!verbose) return false;
    logf("[il2cpp] checked %zu modules, il2cpp_domain_get symbol missing", mods.size());
    if (hits[0]) logf("[il2cpp] name string only in %s (not the export)", hits);
    else logf("[il2cpp] name string not in any module file");
    return false;
}

struct Cand {
    pid_t pid;
    int score;
    char cmd[180];
    uintptr_t unity;
    uintptr_t il2;
    uintptr_t anogs;
};

void attach_reset() {
    g_pid = 0;
    g_anogs = 0;
    g_unity = 0;
    g_il2 = 0;
    g_domainGet = 0;
    g_domain = 0;
    g_libName[0] = 0;
    g_scanned = false;
    g_seenMods = -1;
    g_unityIl2 = false;
    g_state.store(0);
}

uintptr_t find_root_domain(pid_t pid) {
    const char *needle = "IL2CPP Root Domain";
    const size_t nlen = std::strlen(needle);
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
    FILE *fp = std::fopen(path, "r");
    if (!fp) return 0;
    uintptr_t strAt = 0;
    char chunk[65536 + 64];
    logf("[il2cpp] scanning libunity for IL2CPP Root Domain");
    char line[768];
    while (!strAt && std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0;
        char perms[8] = {};
        char file[512] = {};
        file[0] = 0;
        if (std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %511[^\n]", &start, &end, perms, file) < 3)
            continue;
        if (perms[0] != 'r' || end <= start) continue;
        if (!std::strstr(file, "libunity.so") && !std::strstr(line, "il2cpp")) continue;
        size_t keep = 0;
        for (uintptr_t addr = (uintptr_t)start; addr < (uintptr_t)end;) {
            size_t n = (uintptr_t)end - addr;
            if (n > 65536) n = 65536;
            if (!mem_read(pid, addr, chunk + keep, n)) break;
            const size_t total = keep + n;
            bool found = false;
            size_t at = 0;
            if (total >= nlen) {
                for (size_t i = 0; i + nlen <= total; ++i) {
                    if (std::memcmp(chunk + i, needle, nlen) == 0) {
                        found = true;
                        at = i;
                        break;
                    }
                }
            }
            if (found) {
                strAt = addr - keep + at;
                break;
            }
            keep = nlen > 1 ? nlen - 1 : 0;
            if (keep && total >= keep) std::memmove(chunk, chunk + total - keep, keep);
            addr += n;
        }
    }
    if (!strAt) {
        std::fclose(fp);
        logf("[il2cpp] IL2CPP Root Domain not in libunity");
        return 0;
    }
    logf("[il2cpp] root name @ 0x%llx", (unsigned long long)strAt);
    g_unityIl2 = true;
    uintptr_t copies[4] = {strAt, 0, 0, 0};
    int ncopies = 1;
    auto rw_region = [](const char *perms, const char *file) {
        if (perms[0] != 'r' || perms[1] != 'w') return false;
        if (std::strstr(file, "/system/") || std::strstr(file, "/apex/") || std::strstr(file, "/vendor/") ||
            std::strstr(file, "[stack") || std::strstr(file, "/dev/"))
            return false;
        return true;
    };
    std::rewind(fp);
    while (ncopies < 4 && std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0;
        char perms[8] = {};
        char file[512] = {};
        file[0] = 0;
        if (std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %511[^\n]", &start, &end, perms, file) < 3)
            continue;
        if (!rw_region(perms, file) || end <= start) continue;
        if ((size_t)(end - start) > 512u * 1024u * 1024u) continue;
        size_t keep = 0;
        for (uintptr_t addr = (uintptr_t)start; addr < (uintptr_t)end && ncopies < 4;) {
            size_t n = (uintptr_t)end - addr;
            if (n > 65536) n = 65536;
            if (!mem_read(pid, addr, chunk + keep, n)) break;
            const size_t total = keep + n;
            if (total >= nlen) {
                for (size_t i = 0; i + nlen <= total && ncopies < 4; ++i) {
                    if (std::memcmp(chunk + i, needle, nlen) != 0) continue;
                    uintptr_t at = addr - keep + i;
                    bool seen = false;
                    for (int c = 0; c < ncopies; ++c)
                        if (copies[c] == at) seen = true;
                    if (!seen) copies[ncopies++] = at;
                }
            }
            keep = nlen > 1 ? nlen - 1 : 0;
            if (keep && total >= keep) std::memmove(chunk, chunk + total - keep, keep);
            addr += n;
        }
    }
    std::rewind(fp);
    uintptr_t hits[12] = {};
    int nhits = 0;
    size_t scanned = 0;
    logf("[il2cpp] scanning rw memory for domain");
    while (nhits < 12 && std::fgets(line, sizeof(line), fp)) {
        unsigned long start = 0, end = 0;
        char perms[8] = {};
        char file[512] = {};
        file[0] = 0;
        if (std::sscanf(line, "%lx-%lx %7s %*s %*s %*s %511[^\n]", &start, &end, perms, file) < 3)
            continue;
        if (!rw_region(perms, file) || end <= start) continue;
        if ((size_t)(end - start) > 512u * 1024u * 1024u) continue;
        uintptr_t addr = (uintptr_t)start;
        if (addr & 7) addr = (addr + 7) & ~7ull;
        while (addr + 8 <= (uintptr_t)end && nhits < 12) {
            size_t n = (uintptr_t)end - addr;
            if (n > 65536) n = 65536;
            n &= ~7u;
            if (n < 8) break;
            if (!mem_read(pid, addr, chunk, n)) break;
            scanned += n;
            for (size_t i = 0; i + 8 <= n && nhits < 12; i += 8) {
                uintptr_t val = 0;
                std::memcpy(&val, chunk + i, 8);
                for (int c = 0; c < ncopies; ++c)
                    if (val == copies[c]) hits[nhits++] = addr + i;
            }
            addr += n;
        }
    }
    std::fclose(fp);
    logf("[il2cpp] rw scanned=%zuMB copies=%d ptrs=%d", scanned / (1024u * 1024u), ncopies, nhits);
    const int offs[] = {0x0, 0x8, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38};
    for (int h = 0; h < nhits; ++h) {
        for (int off : offs) {
            if (hits[h] < (uintptr_t)off) continue;
            uintptr_t dom = hits[h] - (uintptr_t)off;
            if (dom & 7) continue;
            uintptr_t words[8] = {};
            if (off / 8 >= 8) continue;
            if (!mem_read(pid, dom, words, sizeof(words))) continue;
            bool nameMatch = false;
            for (int c = 0; c < ncopies; ++c)
                if (words[off / 8] == copies[c]) nameMatch = true;
            if (!nameMatch) continue;
            int ptrs = 0;
            for (uintptr_t w : words)
                if (mem_ptr_ok(w)) ++ptrs;
            if (ptrs < 1) continue;
            logf("[il2cpp] domain=0x%llx friendly+%#x", (unsigned long long)dom, off);
            return dom;
        }
    }
    logf("[il2cpp] root name found, domain object not found (%d ptrs)", nhits);
    return 0;
}

} // namespace

const char *attach_status_text() {
    static char local[sizeof(g_status)];
    std::lock_guard<std::mutex> lock(g_mu);
    std::snprintf(local, sizeof(local), "%s", g_status);
    return local;
}

void attach_log_copy(char *dst, size_t cap) {
    if (!dst || !cap) return;
    dst[0] = 0;
    std::lock_guard<std::mutex> lock(g_mu);
    size_t used = 0;
    for (size_t i = 0; i < g_count; ++i) {
        const char *ln = g_lines[(g_start + i) % kMaxLines];
        const size_t n = std::strlen(ln);
        if (used + n + 2 >= cap) break;
        if (i) dst[used++] = '\n';
        std::memcpy(dst + used, ln, n);
        used += n;
    }
    dst[used] = 0;
}

int attach_state() { return g_state.load(); }

void attach_log_line(const char *line) {
    if (line && line[0]) logf("%s", line);
}

void attach_hint_pid(int pid, const char *cmd) {
    if (pid <= 0) return;
    g_hintPid = (pid_t)pid;
    std::snprintf(g_hintCmd, sizeof(g_hintCmd), "%s", cmd ? cmd : "");
    logf("[java] pid=%d %s", pid, g_hintCmd);
}

static pid_t pid_from_cmd(const char *cmd) {
    FILE *fp = popen(cmd, "r");
    if (!fp) {
        logf("[scan] popen fail %s", cmd);
        return 0;
    }
    char buf[128] = {};
    char *got = std::fgets(buf, sizeof(buf), fp);
    int rc = pclose(fp);
    int pid = got ? std::atoi(buf) : 0;
    logf("[scan] %s -> %d rc=%d", cmd, pid, rc);
    return pid > 0 ? (pid_t)pid : 0;
}

void attach_tick() {
    static std::atomic<int> lock{0};
    if (lock.exchange(1)) return;
    logf("[scan] reading /proc for %s", kPkg);

    DIR *d = opendir("/proc");
    if (!d) {
        set_status("Waiting for CODM");
        logf("[scan] cannot open /proc");
        attach_reset();
        lock.store(0);
        return;
    }

    Cand best{};
    int visible = 0;
    int matched = 0;
    int mapsOpened = 0;
    int samples = 0;
    struct dirent *ent;
    auto consider = [&](pid_t pid) {
        if (pid <= 0) return;
        char cmd[180] = {};
        read_cmdline(pid, cmd, sizeof(cmd));
        const bool cmdHit = std::strstr(cmd, kPkg) != nullptr;
        bool mapsPkg = false;
        uintptr_t unity = 0, il2 = 0, anogs = 0;
        char path[64];
        std::snprintf(path, sizeof(path), "/proc/%d/maps", (int)pid);
        FILE *fp = std::fopen(path, "r");
        if (fp) {
            ++mapsOpened;
            char line[768];
            auto has_ci = [](const char *s, const char *needle) {
                for (; *s; ++s) {
                    const char *a = s;
                    const char *b = needle;
                    while (*a && *b && ((*a | 32) == (*b | 32))) {
                        ++a;
                        ++b;
                    }
                    if (!*b) return true;
                }
                return false;
            };
            while (std::fgets(line, sizeof(line), fp)) {
                if (!unity && std::strstr(line, "libunity.so"))
                    unity = (uintptr_t)std::strtoull(line, nullptr, 16);
                if (has_ci(line, "il2cpp")) {
                    char show[200];
                    std::snprintf(show, sizeof(show), "%s", line);
                    for (char *p = show; *p; ++p)
                        if (*p == '\n' || *p == '\r') *p = 0;
                    logf("[map] %s", show);
                    if (!il2 && (std::strstr(line, ".so") || std::strstr(line, "memfd")))
                        il2 = (uintptr_t)std::strtoull(line, nullptr, 16);
                }
                if (!anogs && std::strstr(line, "libanogs.so"))
                    anogs = (uintptr_t)std::strtoull(line, nullptr, 16);
                if (std::strstr(line, kPkg)) mapsPkg = true;
            }
            std::fclose(fp);
        }
        if (cmd[0] && !cmdHit && !unity && !anogs && !il2) return;
        if (!cmdHit && !mapsPkg && !anogs && !unity && !il2) {
            if (samples < 6 && cmd[0] && (std::strstr(cmd, "game") || std::strstr(cmd, "unity") ||
                                          std::strstr(cmd, "garena") || std::strstr(cmd, "cod"))) {
                logf("[proc] pid=%d %s", (int)pid, cmd);
                ++samples;
            }
            return;
        }
        ++matched;
        int score = 0;
        if (il2) score += 100;
        if (unity) score += 50;
        if (anogs) score += 40;
        if (cmdHit) score += 200;
        logf("[hit] pid=%d score=%d unity=%d il2=%d anogs=%d %s", (int)pid, score, unity ? 1 : 0,
             il2 ? 1 : 0, anogs ? 1 : 0, cmd[0] ? cmd : "(no cmdline)");
        if (!best.pid || score > best.score) {
            best.pid = pid;
            best.score = score;
            best.unity = unity;
            best.il2 = il2;
            best.anogs = anogs;
            std::snprintf(best.cmd, sizeof(best.cmd), "%s", cmd);
        }
    };
    while ((ent = readdir(d)) != nullptr) {
        if (ent->d_name[0] < '1' || ent->d_name[0] > '9') continue;
        pid_t pid = (pid_t)std::atoi(ent->d_name);
        if (pid <= 0) continue;
        ++visible;
        consider(pid);
    }
    closedir(d);

    if (best.score < 100) {
        static int aux = 0;
        if ((++aux % 4) == 1) {
            pid_t extra = pid_from_cmd("pidof com.garena.game.codm");
            if (!extra) extra = pid_from_cmd("su -c 'pidof com.garena.game.codm'");
            if (extra) consider(extra);
        }
        if (g_hintPid) consider(g_hintPid);
    }

    if (!best.pid) {
        codm_esp_reset();
        attach_reset();
        set_status("Waiting visible=%d maps=%d", visible, mapsOpened);
        logf("[scan] no CODM  visible=%d maps=%d pkg=%s", visible, mapsOpened, kPkg);
        lock.store(0);
        return;
    }

    const bool pickChanged = best.pid != g_pid || best.unity != g_unity || best.il2 != g_il2;
    if (pickChanged) {
        g_domainGet = 0;
        g_domain = 0;
        g_libName[0] = 0;
        g_scanned = false;
        g_seenMods = -1;
        g_unityIl2 = false;
        codm_esp_reset();
    }
    g_pid = best.pid;
    g_unity = best.unity;
    g_il2 = best.il2;
    g_anogs = best.anogs;
    g_state.store(g_domainGet ? 2 : 1);
    if (pickChanged)
        logf("[scan] visible=%d matched=%d pick=%d unity=%d il2=%d anogs=%d %s", visible, matched,
             (int)best.pid, best.unity ? 1 : 0, best.il2 ? 1 : 0, best.anogs ? 1 : 0,
             best.cmd[0] ? best.cmd : "(no cmdline)");
    set_status("CODM pid %d", (int)best.pid);
    if (g_anogs)
        logf("[anogs] pid=%d base=0x%llx", (int)best.pid, (unsigned long long)g_anogs);
    else
        logf("[anogs] pid=%d not mapped yet", (int)best.pid);

    if (!g_domainGet && (best.unity || best.il2)) {
        static int retry = 0;
        uintptr_t probeAt = best.il2 ? best.il2 : best.unity;
        uint8_t probe = 0;
        if (!mem_read(best.pid, probeAt, &probe, 1)) {
            logf("[il2cpp] process_vm_readv failed errno=%d", errno);
        } else {
            std::vector<Mod> mods;
            if (!collect_mods(best.pid, mods)) {
                logf("[il2cpp] pid=%d maps unreadable errno=%d", (int)best.pid, errno);
            } else {
                ++retry;
                const bool verbose = g_seenMods != (int)mods.size();
                if (verbose && !g_unityIl2) {
                    logf("[il2cpp] pid=%d modules=%zu libunity", (int)best.pid, mods.size());
                    find_domain_get(best.pid, mods, g_domainGet, g_libName, sizeof(g_libName), true);
                    if (!g_domainGet) log_disk_siblings(mods);
                    g_seenMods = (int)mods.size();
                } else if (verbose) {
                    g_seenMods = (int)mods.size();
                }
                if (!g_unityIl2 && !g_domainGet && !g_domain && (verbose || (retry % 3) == 0))
                    scan_rx_elfs(best.pid, g_domainGet, g_libName, sizeof(g_libName));
                if (g_domainGet && !g_domain) {
                    if (domain_from_getter(best.pid, g_domainGet, g_domain))
                        logf("[il2cpp] domain=0x%llx", (unsigned long long)g_domain);
                    else
                        logf("[il2cpp] domain decode pending");
                }
                if (!g_domain && (verbose || (g_unityIl2 && (retry % 8) == 1))) {
                    uintptr_t dom = find_root_domain(best.pid);
                    if (dom) {
                        g_domain = dom;
                        std::snprintf(g_libName, sizeof(g_libName), "libunity.so");
                    }
                }
                if (!g_domain && !g_domainGet && verbose) {
                    if (g_unityIl2)
                        logf("[il2cpp] libunity is il2cpp, domain pending");
                    else
                        logf("[il2cpp] still waiting for libil2cpp to map");
                }
            }
        }
    } else if (!best.unity && !best.il2) {
        logf("[il2cpp] pid=%d waiting for libunity/libil2cpp", (int)best.pid);
    }

    if (g_domain || g_domainGet) {
        g_state.store(2);
        set_status("Resolved libunity pid %d", (int)best.pid);
    } else if (g_unityIl2) {
        g_state.store(1);
        set_status("libunity il2cpp pid %d", (int)best.pid);
    } else if (best.unity || best.il2) {
        g_state.store(1);
        set_status("CODM pid %d  waiting for il2cpp", (int)best.pid);
    } else {
        g_state.store(1);
        set_status("CODM pid %d  libs not mapped", (int)best.pid);
    }
    if (best.unity) codm_esp_resolve(best.pid);
    lock.store(0);
}
