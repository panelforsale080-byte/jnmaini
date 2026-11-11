// dos.h — powerful compile-time string obfuscator (rename of xoxo_plus -> DOS)
// C++14 header-only. Use DOS("literal") to obfuscate string literals.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <mutex>
#include <utility>
#include <type_traits>
#include <cstring>
#include <atomic>

#ifndef DOS_DEFAULT_KEY
// default key: generated from line number and __COUNTER__ so repeated uses differ
// You can override before including by defining DOS_DEFAULT_KEY to a 64-bit literal.
#define DOS_DEFAULT_KEY dos::generate_key((uint64_t)(__LINE__ * 1315423911u + (__COUNTER__)))
#endif

namespace dos
{
    using u64 = uint64_t;
    using u8  = uint8_t;
    using size_type = std::size_t;

    // constexpr SplitMix64 (good distribution; usable in C++14 constexpr)
    constexpr u64 splitmix64_step(u64& x) {
        x += 0x9E3779B97F4A7C15ULL;
        u64 z = x;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        z = z ^ (z >> 31);
        return z;
    }

    // generate a 64-bit key from a seed (mixing)
    constexpr u64 generate_key(u64 seed) {
        u64 x = seed ^ 0x9e3779b97f4a7c15ULL;
        // call the mixer a few times to spread bits
        u64 a = splitmix64_step(x);
        u64 b = splitmix64_step(x);
        return a ^ (b << 1);
    }

    // Produce a keystream byte (from state)
    constexpr u8 keystream_byte(u64& state) {
        u64 block = splitmix64_step(state);
        return static_cast<u8>(block & 0xFFu);
    }

    // constexpr helper to swap two indices in array (used for compile-time shuffle)
    template <size_type N>
    constexpr void cswap(size_type i, size_type j, size_type (&arr)[N]) {
        size_type tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }

    // compile-time Fisher-Yates shuffle to produce a permutation array
    template <size_type N>
    constexpr void make_permutation(size_type (&perm)[N], u64 seed)
    {
        for (size_type i = 0; i < N; ++i) perm[i] = i;
        u64 st = seed;
        // shuffle from last to first
        for (size_type i = N - 1; i > 0; --i) {
            u64 r = splitmix64_step(st);
            size_type j = static_cast<size_type>(r % (i + 1));
            cswap(j, i, perm);
        }
    }

    // secure zeroing helper (best-effort)
    inline void secure_zero(void* p, size_type n) {
        volatile u8* vp = reinterpret_cast<volatile u8*>(p);
        while (n--) *vp++ = 0;
    }

    // obfuscator: executed in constexpr context to produce obfuscated buffer and permutation
    template <size_type N, u64 KEY>
    struct obfuscator
    {
        // storage for obfuscated bytes (scrambled order)
        char data[N];

        // permutation to map original index -> scrambled index
        size_type perm[N];

        // construct constexpr: copy, keystream-xor, then permute storage
        constexpr obfuscator(const char (&s)[N]) : data(), perm()
        {
            // produce permutation (scrambles storage)
            make_permutation<N>(perm, KEY ^ 0xC0FFEE123456789ULL);

            // prepare keystream state seeded by KEY and string length
            u64 st = KEY ^ static_cast<u64>(N * 0x9e3779b97f4a7c15ULL);

            // produce keystream and store bytes into scrambled positions
            for (size_type i = 0; i < N; ++i) {
                u8 k = keystream_byte(st);
                char ob = static_cast<char>(static_cast<u8>(s[i]) ^ k);
                data[perm[i]] = ob;
            }
        }
    };

    // runtime holder: holds scrambled obfuscated bytes and decrypts lazily
    template <size_type N, u64 KEY>
    class obfuscated_data
    {
    public:
        obfuscated_data(const obfuscator<N, KEY>& obf)
        {
            for (size_type i = 0; i < N; ++i) {
                m_scrambled[i] = static_cast<u8>(obf.data[i]);
                m_perm[i] = obf.perm[i];
            }
            // mark encrypted
            m_encrypted.store(true, std::memory_order_relaxed);
        }

        ~obfuscated_data()
        {
            // zero memory (best-effort)
            secure_zero(m_plain, N);
            secure_zero(m_scrambled, N);
            m_encrypted.store(true, std::memory_order_relaxed);
        }

        // returns mutable char*; decrypts on first call (thread-safe)
        operator char*()
        {
            decrypt();
            return m_plain;
        }

        operator std::string()
        {
            decrypt();
            // exclude terminating '\0'
            return std::string(m_plain, N ? (N - 1) : 0);
        }

        bool is_encrypted() const { return m_encrypted.load(std::memory_order_relaxed); }

        // explicit control
        void decrypt()
        {
            // double-check locking via std::call_once
            std::call_once(m_once, [this]() {
                if (!m_encrypted.load(std::memory_order_relaxed)) return;
                u64 st = KEY ^ static_cast<u64>(N * 0x9e3779b97f4a7c15ULL);
                for (size_type i = 0; i < N; ++i) {
                    u8 k = static_cast<u8>(splitmix64_step(st) & 0xFFu);
                    size_type scrambled_idx = m_perm[i];
                    u8 ob = m_scrambled[scrambled_idx];
                    m_plain[i] = static_cast<char>(ob ^ k);
                }
                m_encrypted.store(false, std::memory_order_relaxed);
            });
        }

        void encrypt()
        {
            // only re-encrypt when decrypted
            if (m_encrypted.load(std::memory_order_relaxed)) return;
            u64 st = KEY ^ static_cast<u64>(N * 0x9e3779b97f4a7c15ULL);
            for (size_type i = 0; i < N; ++i) {
                u8 k = static_cast<u8>(splitmix64_step(st) & 0xFFu);
                size_type scrambled_idx = m_perm[i];
                m_scrambled[scrambled_idx] = static_cast<u8>(m_plain[i] ^ k);
            }
            secure_zero(m_plain, N);
            m_encrypted.store(true, std::memory_order_relaxed);
            m_once = std::once_flag(); // allow decrypt again later
        }

    private:
        u8 m_scrambled[N];
        size_type m_perm[N];
        char m_plain[N];
        std::atomic<bool> m_encrypted{ true };
        std::once_flag m_once;
    };

    // helper factory to build obfuscator at compile time
    template <size_type N, u64 KEY = DOS_DEFAULT_KEY>
    constexpr obfuscator<N, KEY> make_obfuscator(const char (&s)[N])
    {
        return obfuscator<N, KEY>(s);
    }
} // namespace dos

// Macro wrappers — ergonomic: DOS_KEY("literal", key) or DOS("literal")
// Note: macros take string literal (not std::string or char* variables)
#define DOS_KEY(s, key) \
    []() -> dos::obfuscated_data<sizeof(s)/sizeof(s[0]), key>& { \
        static_assert(sizeof(decltype(key)) == sizeof(dos::u64), "key must be 64-bit"); \
        constexpr auto n = sizeof(s)/sizeof(s[0]); \
        constexpr auto obf = dos::make_obfuscator<n, key>(s); \
        static dos::obfuscated_data<n, key> instance(obf); \
        return instance; \
    }()

#define DOS(s) DOS_KEY(s, DOS_DEFAULT_KEY)

// compatibility macro (lowercase) if you prefer to use DOS(...) like before
#define dos_str(s) DOS(s)
