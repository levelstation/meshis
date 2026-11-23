#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "DiscoveryQueueGuard.h"

struct FakePacket {
  uint8_t payload[8] = {0x80, 0x04, 0x12, 0x34, 0x56, 0x78};
  unsigned payload_len = 6, path_len = 0;
  uint8_t type = 0x0B, route = 2;
  uint8_t getPayloadType() const { return type; }
  uint8_t getRouteType() const { return route; }
};

// This fake mirrors the native manager's remove-without-free and compaction
// semantics. A free while still queued, or a second free, fails immediately.
struct FakeManager {
  std::vector<FakePacket*> queue, freed;
  unsigned removals = 0;
  int getOutboundTotal() const { return int(queue.size()); }
  FakePacket* getOutboundByIdx(int index) {
    return index < 0 || unsigned(index) >= queue.size() ? nullptr : queue[index];
  }
  FakePacket* removeOutboundByIdx(int index) {
    auto* packet = getOutboundByIdx(index);
    if (!packet) return nullptr;
    queue.erase(queue.begin() + index);
    ++removals;
    return packet;
  }
  void free(FakePacket* packet) {
    assert(packet);
    for (const auto* queued : queue) assert(queued != packet);
    for (const auto* previous : freed) assert(previous != packet);
    freed.push_back(packet);
  }
};

static void removesOnlyOwnedRequestAndCompacts() {
  meshis::DiscoveryQueueGuard guard;
  FakePacket owned, before, sameTagBle, after;
  before.type = 0x05;
  after.type = 0x02;
  FakeManager manager;
  manager.queue = {&before, &owned, &sameTagBle, &after};
  assert(!guard.pending());
  assert(guard.requestTag() == 0);
  assert(!guard.cancel(manager));
  guard.track(&owned);
  assert(guard.pending());
  assert(guard.requestTag() == 0x78563412);
  guard.finished(&sameTagBle); // Identical bytes/tag do not establish ownership.
  assert(guard.pending());
  assert(guard.cancel(manager));
  assert(!guard.pending() && manager.removals == 1);
  assert(guard.requestTag() == 0);
  assert(manager.freed.size() == 1 && manager.freed[0] == &owned);
  assert(manager.queue.size() == 3 && manager.queue[0] == &before);
  assert(manager.queue[1] == &sameTagBle && manager.queue[2] == &after);
  assert(!guard.cancel(manager));
  assert(manager.freed.size() == 1 && manager.removals == 1);
}

static void absentAndInflightNeverFree() {
  meshis::DiscoveryQueueGuard guard;
  FakePacket owned, ble;
  FakeManager manager;
  manager.queue = {&owned, &ble};
  guard.track(&owned);
  // Native Dispatcher removes the packet before starting RF transmission.
  assert(manager.removeOutboundByIdx(0) == &owned);
  assert(!guard.cancel(manager));
  assert(!guard.pending() && !guard.requestTag() && manager.freed.empty());
  assert(manager.queue.size() == 1 && manager.queue[0] == &ble);
  assert(!guard.cancel(manager) && manager.freed.empty());
  guard.finished(&ble);
  assert(!guard.pending());
  guard.finished(&owned); // Both success and failure hooks use this operation.
  assert(!guard.pending());
  assert(!guard.cancel(manager) && manager.freed.empty());

  // Even identical bytes at the old pointer are safe after its completion hook.
  manager.queue.push_back(&owned);
  assert(!guard.cancel(manager));
  assert(manager.queue.size() == 2 && manager.freed.empty());

  meshis::DiscoveryQueueGuard absent;
  absent.track(&owned);
  FakeManager empty;
  assert(!absent.cancel(empty) && empty.freed.empty());
  assert(!absent.pending() && !absent.requestTag());
  absent.finished(&owned);
  assert(!absent.pending());
}

static void reusedOrMutatedPointerDoesNotCancel() {
  for (unsigned scenario = 0; scenario < 11; ++scenario) {
    meshis::DiscoveryQueueGuard guard;
    FakePacket reused, other;
    guard.track(&reused);
    switch (scenario) {
      case 0: reused.type = 0x05; break;
      case 1: reused.route = 0; break;
      case 2: reused.route = 1; break;
      case 3: reused.route = 3; break;
      case 4: reused.path_len = 1; break;
      case 5: reused.path_len = 0x40; break; // Zero count still differs from ours.
      case 6: ++reused.payload[2]; break;
      case 7: reused.payload_len = 5; break;
      case 8: reused.payload_len = 7; break;
      case 9: reused.payload[0] = 0x81; break;
      case 10: reused.payload[1] = 0xFF; break;
    }
    FakeManager manager;
    manager.queue = {&other, &reused};
    assert(!guard.cancel(manager));
    assert(!guard.pending() && !guard.requestTag());
    assert(manager.removals == 0 && manager.freed.empty());
    assert(manager.queue.size() == 2 && manager.queue[0] == &other);
    assert(manager.queue[1] == &reused);
    assert(!guard.cancel(manager) && manager.freed.empty());
  }
}

static void completionClearsOnlyItsOwnedPointer() {
  meshis::DiscoveryQueueGuard guard;
  FakePacket owned, unrelated;
  FakeManager manager;
  manager.queue = {&owned, &unrelated};
  guard.track(&owned);
  guard.finished(nullptr);
  guard.finished(&unrelated);
  assert(guard.pending());
  guard.finished(&owned);
  assert(!guard.pending());
  assert(!guard.cancel(manager));
  assert(manager.queue.size() == 2 && manager.freed.empty() && !manager.removals);
  guard.track(static_cast<FakePacket*>(nullptr));
  assert(!guard.pending() && !guard.cancel(manager));
}

int main() {
  removesOnlyOwnedRequestAndCompacts();
  absentAndInflightNeverFree();
  reusedOrMutatedPointerDoesNotCancel();
  completionClearsOnlyItsOwnedPointer();
  puts("PASS: actual DiscoveryQueueGuard; only exact owned queued request is "
       "removed/freed once, BLE same-tag requests survive, queue compacts, "
       "in-flight/absent/reused packets remain safe. Fake manager only.");
}
