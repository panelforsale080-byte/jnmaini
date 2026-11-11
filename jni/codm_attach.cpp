#include "codm_attach.h"

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
    if (std::strstr(base, "il2cpp")) return 100;
    if (std::strstr(base, "unity")) return 80;
    if (std::strstr(base, "GameAssembly")) return 70;
    return 10;
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
        if (std::strcmp(strData + s.st_name, sym) != 0) continue;
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
    if (minV == UINTPTR_MAX || eh.e_shnum == 0 || eh.e_shnum > 256 || !eh.e_shoff) {
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
    for (const Elf64_Shdr &s : sh) {
        if (s.sh_name >= names.size()) continue;
        const char *nm = names.data() + s.sh_name;
        if (std::strcmp(nm, ".dynsym") == 0) dynsym = &s;
        if (std::strcmp(nm, ".dynstr") == 0) dynstr = &s;
    }
    if (!dynsym || !dynstr || dynsym->sh_entsize != sizeof(Elf64_Sym) || !dynstr->sh_size ||
        dynstr->sh_size > 32u * 1024u * 1024u || dynsym->sh_size > 32u * 1024u * 1024u) {
        std::fclose(f);
        return false;
    }
    std::vector<char> str(dynstr->sh_size);
    std::vector<Elf64_Sym> syms(dynsym->sh_size / sizeof(Elf64_Sym));
    bool ok = std::fseek(f, (long)dynstr->sh_offset, SEEK_SET) == 0 &&
              std::fread(str.data(), 1, str.size(), f) == str.size() &&
              std::fseek(f, (long)dynsym->sh_offset, SEEK_SET) == 0 &&
              std::fread(syms.data(), sizeof(Elf64_Sym), syms.size(), f) == syms.size() &&
              lookup_syms(sym, str.data(), str.size(), syms.data(), syms.size(), bias, addr);
    std::fclose(f);
    return ok;
}

bool find_domain_get(pid_t pid, const std::vector<Mod> &mods, uintptr_t &addr, char *name, size_t nameCap) {
    addr = 0;
    int tried = 0;
    for (const Mod &m : mods) {
        if (m.score < 70 && tried > 0) break;
        ++tried;
        const char *base = std::strrchr(m.path, '/');
        base = base ? base + 1 : m.path;
        errno = 0;
        if (sym_from_file(m.path, m.base, "il2cpp_domain_get", addr)) {
            std::snprintf(name, nameCap, "%s", base);
            logf("[il2cpp] file %s base=0x%llx domain_get=0x%llx", base,
                 (unsigned long long)m.base, (unsigned long long)addr);
            return true;
        }
        const int fileErr = errno;
        errno = 0;
        if (sym_from_dynamic(pid, m.base, "il2cpp_domain_get", addr)) {
            std::snprintf(name, nameCap, "%s", base);
            logf("[il2cpp] dynsym %s base=0x%llx domain_get=0x%llx", base,
                 (unsigned long long)m.base, (unsigned long long)addr);
            return true;
        }
        logf("[il2cpp] no export in %s file=%d read=%d", base, fileErr, errno);
        if (tried >= 4) break;
    }
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
    g_state.store(0);
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
            while (std::fgets(line, sizeof(line), fp)) {
                if (!unity && std::strstr(line, "libunity.so"))
                    unity = (uintptr_t)std::strtoull(line, nullptr, 16);
                if (!il2 && std::strstr(line, "libil2cpp.so"))
                    il2 = (uintptr_t)std::strtoull(line, nullptr, 16);
                if (!anogs && std::strstr(line, "libanogs.so"))
                    anogs = (uintptr_t)std::strtoull(line, nullptr, 16);
                if (std::strstr(line, kPkg)) mapsPkg = true;
            }
            std::fclose(fp);
        }
        if (!cmdHit && !mapsPkg && !anogs && !(unity && il2)) {
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
        if (cmdHit || mapsPkg) score += 200;
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
        attach_reset();
        set_status("Waiting visible=%d maps=%d", visible, mapsOpened);
        logf("[scan] no CODM  visible=%d maps=%d pkg=%s", visible, mapsOpened, kPkg);
        lock.store(0);
        return;
    }

    if (best.pid != g_pid || best.unity != g_unity || best.il2 != g_il2) {
        g_domainGet = 0;
        g_domain = 0;
        g_libName[0] = 0;
        g_scanned = false;
    }
    g_pid = best.pid;
    g_unity = best.unity;
    g_il2 = best.il2;
    g_anogs = best.anogs;
    g_state.store(g_domainGet ? 2 : 1);
    logf("[scan] visible=%d matched=%d pick=%d unity=%d il2=%d anogs=%d %s", visible, matched,
         (int)best.pid, best.unity ? 1 : 0, best.il2 ? 1 : 0, best.anogs ? 1 : 0,
         best.cmd[0] ? best.cmd : "(no cmdline)");
    set_status("CODM pid %d", (int)best.pid);
    if (g_anogs)
        logf("[anogs] pid=%d base=0x%llx", (int)best.pid, (unsigned long long)g_anogs);
    else
        logf("[anogs] pid=%d not mapped yet", (int)best.pid);

    if (!g_domainGet && (best.unity || best.il2) && !g_scanned) {
        uintptr_t probeAt = best.il2 ? best.il2 : best.unity;
        uint8_t probe = 0;
        if (!mem_read(best.pid, probeAt, &probe, 1)) {
            logf("[il2cpp] process_vm_readv failed errno=%d", errno);
        } else {
            std::vector<Mod> mods;
            if (!collect_mods(best.pid, mods)) {
                logf("[il2cpp] pid=%d maps unreadable errno=%d", (int)best.pid, errno);
            } else {
                logf("[il2cpp] pid=%d modules=%zu", (int)best.pid, mods.size());
                find_domain_get(best.pid, mods, g_domainGet, g_libName, sizeof(g_libName));
                if (g_domainGet) {
                    if (domain_from_getter(best.pid, g_domainGet, g_domain))
                        logf("[il2cpp] domain=0x%llx", (unsigned long long)g_domain);
                    else
                        logf("[il2cpp] domain decode pending");
                } else {
                    logf("[il2cpp] il2cpp_domain_get not found");
                }
                g_scanned = true;
            }
        }
    } else if (!best.unity && !best.il2) {
        logf("[il2cpp] pid=%d waiting for libunity/libil2cpp", (int)best.pid);
    }

    if (g_domainGet) {
        g_state.store(2);
        set_status("Resolved %s pid %d", g_libName[0] ? g_libName : "il2cpp", (int)best.pid);
    } else if (g_scanned) {
        g_state.store(1);
        set_status("CODM pid %d  il2cpp export missing", (int)best.pid);
    } else if (best.unity || best.il2) {
        g_state.store(1);
        set_status("CODM pid %d  resolving il2cpp", (int)best.pid);
    } else {
        g_state.store(1);
        set_status("CODM pid %d  libs not mapped", (int)best.pid);
    }
    lock.store(0);
}
