#pragma once
//
// hp/chunk_table.hpp — demand-allocated fixed tables for context/match slots.
//
// A full 2^22 slot table costs 8 MB up front even when most chunks are never
// touched on a short proxy. Chunks are allocated on first write; untouched
// slots read as zero, matching eager zero-init.

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "hp/features.hpp"

namespace hp {

#ifndef HP_TABLE_CHUNK_BITS
#define HP_TABLE_CHUNK_BITS 12   // 4096 slots per chunk
#endif

template <typename T>
class ChunkTable {
 public:
    static_assert(sizeof(T) == 2 || sizeof(T) == 4, "ChunkTable element size");

    explicit ChunkTable(int table_bits)
        : mask_((1u << table_bits) - 1),
          chunk_mask_((1u << HP_TABLE_CHUNK_BITS) - 1),
          shift_(table_bits - HP_TABLE_CHUNK_BITS),
          chunks_(static_cast<std::size_t>(1) << shift_) {}

    T get(std::uint32_t idx) const {
        idx &= mask_;
        const std::size_t cid = idx >> HP_TABLE_CHUNK_BITS;
        const T* chunk = chunks_[cid].get();
        return chunk ? chunk[idx & chunk_mask_] : T{};
    }

    T& ref(std::uint32_t idx) {
        idx &= mask_;
        const std::size_t cid = idx >> HP_TABLE_CHUNK_BITS;
        if (!chunks_[cid]) alloc_chunk(cid);
        return chunks_[cid][idx & chunk_mask_];
    }

    std::size_t allocated_chunks() const {
        std::size_t n = 0;
        for (const auto& c : chunks_)
            if (c) ++n;
        return n;
    }

    std::size_t num_chunks() const { return chunks_.size(); }

 private:
    void alloc_chunk(std::size_t cid) {
        chunks_[cid] = std::make_unique<T[]>(static_cast<std::size_t>(1)
                                             << HP_TABLE_CHUNK_BITS);
        std::memset(chunks_[cid].get(), 0,
                    (static_cast<std::size_t>(1) << HP_TABLE_CHUNK_BITS) * sizeof(T));
    }

    std::uint32_t mask_;
    std::uint32_t chunk_mask_;
    int shift_;
    std::vector<std::unique_ptr<T[]>> chunks_;
};

// Unified get/ref interface: lazy chunks when HP_LAZY_TABLES, else eager vector.
template <typename T>
class HashTable {
 public:
#if HP_LAZY_TABLES
    explicit HashTable(int table_bits) : tab_(table_bits) {}
    T get(std::uint32_t idx) const { return tab_.get(idx); }
    T& ref(std::uint32_t idx) { return tab_.ref(idx); }
    std::size_t allocated_chunks() const { return tab_.allocated_chunks(); }
    std::size_t num_chunks() const { return tab_.num_chunks(); }
#else
    explicit HashTable(int table_bits)
        : mask_((1u << table_bits) - 1),
          tab_(static_cast<std::size_t>(1) << table_bits, 0) {}
    T get(std::uint32_t idx) const { return tab_[idx & mask_]; }
    T& ref(std::uint32_t idx) { return tab_[idx & mask_]; }
#endif

 private:
#if !HP_LAZY_TABLES
    std::uint32_t mask_;
    std::vector<T> tab_;
#else
    ChunkTable<T> tab_;
#endif
};

}  // namespace hp
