// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
// See end of file for extended copyright information.

#pragma once

/**
 * Generic Memory Arena Implementation
 *
 * A memory arena provides fast, sequential allocation with minimal overhead.
 * Memory is allocated from a large buffer and can be reset all at once.
 *
 * Features:
 * - Fast O(1) allocation with pointer arithmetic
 * - Automatic alignment handling
 * - Scoped allocation with ScopedArena RAII wrapper
 * - Zero-initialization support
 * - Memory usage tracking
 * - Reset/clear functionality
 *
 * Basic Usage:
 *   Arena arena(1024 * 1024);  // 1MB arena
 *
 *   int* numbers = arena.allocate<int>(100);           // Allocate 100 ints
 *   char* buffer = arena.allocate<char>(256);          // Allocate 256 chars
 *   MyStruct* obj = arena.allocate<MyStruct>();        // Allocate single object
 *
 *   arena.reset();  // Free all allocations at once
 *
 * Scoped Usage:
 *   Arena arena(1024);
 *   {
 *       ScopedArena scoped(arena);
 *       int* temp = scoped.allocate<int>(50);  // Temporary allocation
 *   } // Automatically freed when scope ends
 *
 * Zero-initialized:
 *   int* zeros = arena.allocate_zero<int>(100);  // All elements set to 0
 */

#include <cstddef>
#include <cstring>
#include <memory>

class Arena final {
public:
  /// Constructor with initial capacity
  explicit Arena(const size_t capacity = 4096) : m_buffer(std::make_unique<std::byte[]>(capacity)), m_capacity(capacity), m_offset(0) {}

  /// Non-copyable but movable
  Arena(const Arena&)            = delete;
  Arena& operator=(const Arena&) = delete;
  Arena(Arena&&)                 = default;
  Arena& operator=(Arena&&)      = default;

  /// Allocate memory for count objects of type T
  template<typename T>
  T* allocate(const size_t count = 1) {
    const size_t size           = sizeof(T) * count;
    const size_t aligned_offset = align_offset(m_offset, alignof(T));

    if (aligned_offset + size > m_capacity)
      return nullptr;   // Out of memory

    m_offset = aligned_offset + size;
    return reinterpret_cast<T*>(m_buffer.get() + aligned_offset);
  }

  /// Allocate and zero-initialize memory
  template<typename T>
  T* allocate_zero(const size_t count = 1) {
    T* result = allocate<T>(count);
    if (result)
      std::memset(result, 0, sizeof(T) * count);
    return result;
  }

  /// Allocate memory with custom alignment
  template<typename T>
  T* allocate_aligned(const size_t count = 1, const size_t alignment = alignof(T)) {
    const size_t size           = sizeof(T) * count;
    const size_t aligned_offset = align_offset(m_offset, alignment);

    if (aligned_offset + size > m_capacity)
      return nullptr;   // Out of memory

    T* result = reinterpret_cast<T*>(m_buffer.get() + aligned_offset);
    m_offset  = aligned_offset + size;
    return result;
  }

  /// Reset the arena, making all memory available again
  void reset() {
    m_offset = 0;
  }

  /// Resize the arena to a new capacity, preserving existing data if possible
  bool resize(const size_t new_capacity) {
    // If new capacity is smaller or equal, no need to resize
    if (new_capacity <= m_capacity)
      return true;

    // Create new buffer with increased capacity
    auto new_buffer = std::make_unique<std::byte[]>(new_capacity);
    if (!new_buffer) {
      return false;   // Failed to allocate new buffer
    }

    // Copy existing data to new buffer if there's any
    if (m_offset > 0 && m_buffer) {
      std::memcpy(new_buffer.get(), m_buffer.get(), m_offset);
    }

    // Replace old buffer with new one
    m_buffer   = std::move(new_buffer);
    m_capacity = new_capacity;

    return true;
  }

  // Resize and reset the arena to a new capacity
  bool resize_and_reset(const size_t new_capacity) {
    if (new_capacity <= m_capacity) {
      // If new capacity is smaller or equal, just reset
      reset();
      return true;
    }

    // Create new buffer with increased capacity
    auto new_buffer = std::make_unique<std::byte[]>(new_capacity);
    if (!new_buffer) {
      return false;   // Failed to allocate new buffer
    }

    // Replace old buffer and reset offset
    m_buffer   = std::move(new_buffer);
    m_capacity = new_capacity;
    m_offset   = 0;

    return true;
  }

  // Get current memory usage
  size_t used() const {
    return m_offset;
  }

  // Get total capacity
  size_t capacity() const {
    return m_capacity;
  }

  // Get remaining free space
  size_t remaining() const {
    return m_capacity - m_offset;
  }

  // Check if arena is empty
  bool empty() const {
    return m_offset == 0;
  }

  // Get current position (for saving/restoring state)
  size_t position() const {
    return m_offset;
  }

  // Restore to a previous position
  void restore(const size_t pos) {
    if (pos <= m_capacity)
      m_offset = pos;
  }

  // Align offset to the specified alignment
  static size_t align_offset(const size_t offset, const size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
  }

  template<typename T>
  static constexpr size_t aligned_sizeof(const size_t count = 1) {
    constexpr size_t alignment = alignof(T);
    constexpr size_t size      = sizeof(T);
    const size_t total         = size * count;
    return (total + alignment - 1) & ~(alignment - 1);
  }

private:
  std::unique_ptr<std::byte[]> m_buffer;
  size_t m_capacity;
  size_t m_offset;
};

/**
 * RAII wrapper for scoped arena allocation
 * Automatically restores arena state when destroyed
 * ONLY USE FOR TEMPORARY ALLOCATIONS
 */
class ScopedArena final {
public:
  explicit ScopedArena(Arena& arena) : m_arena(arena), m_saved_position(arena.position()) {}

  ~ScopedArena() {
    m_arena.restore(m_saved_position);
  }

  // Non-copyable, non-movable
  ScopedArena(const ScopedArena&)            = delete;
  ScopedArena& operator=(const ScopedArena&) = delete;
  ScopedArena(ScopedArena&&)                 = delete;
  ScopedArena& operator=(ScopedArena&&)      = delete;

  // Forward allocation methods to the underlying arena
  template<typename T>
  T* allocate(const size_t count = 1) {
    return m_arena.allocate<T>(count);
  }

  template<typename T>
  T* allocate_zero(const size_t count = 1) {
    return m_arena.allocate_zero<T>(count);
  }

  template<typename T>
  T* allocate_aligned(const size_t count = 1, const size_t alignment = alignof(T)) {
    return m_arena.allocate_aligned<T>(count, alignment);
  }

  // Get the underlying arena
  Arena& arena() {
    return m_arena;
  }

private:
  Arena& m_arena;
  size_t m_saved_position;
};

// Feliscatus, a UCI chess playing engine derived from Tomcat 1.0 (Bobcat 8.0)
// Copyright (C) 2008-2016 Gunnar Harms (Bobcat author)
// Copyright (C) 2017      FireFather (Tomcat author)
// Copyright (C) 2020-2025 Rudy Alex Kohn
//
// Feliscatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Feliscatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Feliscatus.  If not, see <http://www.gnu.org/licenses/>.