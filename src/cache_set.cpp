// This file contains the implementation of the cache set methods
// By: Nick from CoffeeBeforeArch
// 4/16/2020

#include "cache_set.hh"
#include <algorithm>
#include <functional>
#include <numeric>
#include <tuple>
#include "cacheConfig.pb.h"

// Constructor
// Resizes each set based on the number of ways
CacheSet::CacheSet(int ways) {
  lines.resize(ways);
  priority.resize(ways);
  dirty_bits.resize(ways);
}

// Probe
// Looks up an address in the cache set and returns true if found
std::tuple<bool, bool> CacheSet::probe(uint64_t addr, bool type) {
  // Look up the address in the set
  auto result = std::find(begin(lines), end(lines), addr);
  auto hit = result != end(lines);

  // Find the latest line updated
  int latest_line;

  // Replace line if not found
  if (!hit)
    latest_line = replace_line(addr);
  else
    latest_line = std::distance(begin(lines), result);

  // Update the priority
  update_priority(latest_line);

  // Get if their was a dirty writeback
  auto dirty_wb = update_dirty_state(latest_line, type, hit);

  // Return the probe status
  return {hit, dirty_wb};
}

// Replace Line
// Replace a line in the cache
uint32_t CacheSet::replace_line(uint64_t addr) {
  // Check for empty slots
  auto full = used_lines != lines.size();

  // Put the address in the first empty slot
  if (!full) {
    // Place the address, update the used lines, and return
    lines[used_lines] = addr;
    return used_lines++;
  }

  // Find the lowest priority element (the max number)
  auto it = std::max_element(begin(priority), end(priority));

  // Replace the address, and reset the priority
  auto index = std::distance(begin(priority), it);
  lines[index] = addr;
  return index;
}

// Update the priority
// Set the priority of the latest line to 0
void CacheSet::update_priority(uint32_t latest_line) {
  // Update everyone and zero out the other prio
  // Removes a branch
  for (auto &p : priority) p++;
  priority[latest_line] = 0;
}

// Update dirty state
// Updates the dirty bits for the accessed cache line and returns if their was a
// dirty eviction
bool CacheSet::update_dirty_state(uint32_t latest_line, bool type, bool hit) {
  // Check if this caused a dirty cache line to be written back
  // Dirty wb only if line was dirty and it was a miss
  auto dirty_wb = dirty_bits[latest_line] && !hit;

  // Update the dirty state
  // Dirty if it's a write, or any operation to a dirty cache line that's a hit
  dirty_bits[latest_line] = type || (hit && dirty_bits[latest_line]);

  return dirty_wb;
}