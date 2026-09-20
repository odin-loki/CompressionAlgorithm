#pragma once
//
// Demand-zero hash tables. Pages are reserved/committed but not touched, so
// RSS grows with use. Untouched pages read as zero — bit-identical to
// std::vector<T>(n, 0). No prefetch, no compiler attributes.

#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <type_traits>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#endif

namespace hp {

template <typename T>
class HashTable {
    static_assert(std::is_trivially_copyable<T>::value,
                  "HashTable T must be trivially copyable");

 public:
    explicit HashTable(int table_bits)
        : mask_((1u << table_bits) - 1),
          bytes_((static_cast<std::size_t>(1) << table_bits) * sizeof(T)),
          tab_(alloc_zero(bytes_)) {}

    ~HashTable() { free_zero(tab_, bytes_); }

    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;
    HashTable(HashTable&& o) noexcept
        : mask_(o.mask_), bytes_(o.bytes_), tab_(o.tab_) {
        o.tab_ = nullptr;
        o.bytes_ = 0;
    }
    HashTable& operator=(HashTable&&) = delete;

    T get(std::uint32_t idx) const { return tab_[idx & mask_]; }
    T& ref(std::uint32_t idx) { return tab_[idx & mask_]; }
    T& operator[](std::uint32_t idx) { return tab_[idx & mask_]; }
    const T& operator[](std::uint32_t idx) const { return tab_[idx & mask_]; }

 private:
    static T* alloc_zero(std::size_t bytes) {
        if (bytes == 0) return nullptr;
#ifdef _WIN32
        void* p = VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT,
                               PAGE_READWRITE);
        if (!p) std::abort();
#else
        void* p = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) std::abort();
#ifdef MADV_HUGEPAGE
        madvise(p, bytes, MADV_HUGEPAGE);
#endif
#endif
        return static_cast<T*>(p);
    }

    static void free_zero(T* p, std::size_t bytes) {
        if (!p) return;
#ifdef _WIN32
        (void)bytes;
        VirtualFree(p, 0, MEM_RELEASE);
#else
        munmap(p, bytes);
#endif
    }

    std::uint32_t mask_;
    std::size_t bytes_;
    T* tab_;
};

}  // namespace hp
