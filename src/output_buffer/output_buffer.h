
#include <atomic>
#include <cstdint>
#include <array>
#include <bit>

/* This is an ordered container
 * A queue style architecture, aka a ringbuffer, was considered
 * however due to the fact that message processing is unordered,
 * the queing mechanism adds more bookkeeping than is reasonable.

 * This container on the other hand simply processes all "dirty"
 * slots, using a bump style pointer to mark the head of the list.

 * TODO: See if this could be made more generic based on use case.
 * TODO: Look into making the buffer into a single array, delimited
 * by compile time size constants. 

 * Assumptions being made:
   - Multiple threads will not modify the same slot.
   - The full buffer may be edited by multiple threads,
     suppose the case where every message type gets it's own
     thread task in a pool.
 */

template<typename T, uint64_t SLOTSIZE, uint64_t NSLOTS>
requires(std::has_single_bit(SLOTSIZE * NSLOTS))
class FApiBuffer {
  // A slot needs to provide functionality for
  // random access modifications. I.e. We
  // find out an instrument name stored in
  // field 2 is stale, we need to edit with a
  // new value. Move all subsequent fields to make
  // room, or compact if new instrument name is smaller.
  struct Slot {
    uint64_t append_uint64(uint64_t data) {
      this->dirty = true;
    }
    
    std::array<std::byte, SLOTSIZE> data;
    static constexpr uint64_t capacity = SLOTSIZE;
    uint64_t length = 0;
    [[deprecated("Just for prototyping")]]
    bool dirty = false;
  };

  uint64_t allocate_slot() {
    auto slot_idx = this->head.load(std::memory_order_relaxed);
    head.fetch_add(1, std::memory_order_relaxed);
    return slot_idx;
  }

private:
  std::array<Slot, NSLOTS> slots;
  std::atomic<uint64_t> slots_in_flight;
  std::atomic<uint64_t> head;
};
