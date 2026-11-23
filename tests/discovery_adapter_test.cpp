#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

struct FakeRadio {
  uint32_t estimate = 137;
  std::vector<unsigned> lengths;
  uint32_t getEstAirtimeFor(unsigned length) {
    lengths.push_back(length);
    return estimate;
  }
} radio_driver;

struct FakePrefs { float freq = 869.5f, bw = 62.5f; };
struct FakeRng {
  uint8_t bytes[4] = {0x12, 0x34, 0x56, 0x78};
  unsigned calls = 0, size = 0;
  void random(uint8_t* destination, unsigned length) {
    ++calls;
    size = length;
    assert(length == sizeof(bytes));
    memcpy(destination, bytes, length);
  }
};
struct FakePacket { uint8_t payload[6] = {}; unsigned length = 0; };
struct FakeMesh {
  FakePrefs prefs;
  FakeRng rng;
  FakePacket packet;
  bool ready = true, allocation = true, admitted = true;
  unsigned readyCalls = 0, creates = 0, tracks = 0, zeroHops = 0, floods = 0;
  uint32_t readyEstimate = 0;
  FakePacket* sent = nullptr;
  FakePacket* tracked = nullptr;
  FakePrefs* getNodePrefs() { return &prefs; }
  FakeRng* getRNG() { return &rng; }
  bool meshisDiscoveryReady(uint32_t requestMs) {
    ++readyCalls;
    readyEstimate = requestMs;
    return ready;
  }
  FakePacket* createControlData(const uint8_t* bytes, unsigned length) {
    ++creates;
    assert(length == sizeof(packet.payload));
    if (!allocation) return nullptr;
    packet.length = length;
    memcpy(packet.payload, bytes, length);
    return &packet;
  }
  bool meshisTrackDiscovery(FakePacket* value) {
    assert(value == &packet && creates == 1 && zeroHops == 1);
    if (admitted) tracked = value;
    ++tracks;
    return admitted;
  }
  void sendZeroHop(FakePacket* value) {
    assert(value == &packet);
    assert(!tracked && tracks == 0);
    sent = value;
    ++zeroHops;
  }
  void sendFlood(FakePacket*) { ++floods; }
} the_mesh;

// Include the production adapter after its native API dependencies. The
// simulator replacement is never involved in this test.
#include "MeshCoreDiscovery.h"

static const uint8_t request[] = {0x80, 0x04, 0x12, 0x34, 0x56, 0x78};

static void reset() {
  radio_driver = FakeRadio{};
  the_mesh = FakeMesh{};
  meshis::keyLock() = meshis::KeyLock{};
  meshis::activeDiscovery() = meshis::DiscoveryScan{};
  meshis::runtimeTelemetry() = meshis::RuntimeTelemetry{};
}

static void noNativeSend() {
  assert(the_mesh.zeroHops == 0 && the_mesh.floods == 0 && !the_mesh.sent);
  assert(!the_mesh.tracked && the_mesh.tracks == 0);
}

static void savedSettingsEstimateAndReadiness() {
  reset();
  MeshCoreDiscovery adapter;
  assert(adapter.estimateRequestMs() == radio_driver.estimate);
  assert(radio_driver.lengths.size() == 1 && radio_driver.lengths[0] == 8);
  assert(adapter.frequencyMHz() == the_mesh.prefs.freq);
  assert(adapter.bandwidthKHz() == the_mesh.prefs.bw);
  the_mesh.prefs.freq = 868.25f;
  the_mesh.prefs.bw = 125.0f;
  assert(adapter.frequencyMHz() == 868.25f && adapter.bandwidthKHz() == 125.0f);
  assert(adapter.canQueue() && the_mesh.readyCalls == 1);
  assert(the_mesh.readyEstimate == radio_driver.estimate);
  the_mesh.ready = false;
  radio_driver.estimate = 444;
  assert(!adapter.canQueue() && the_mesh.readyCalls == 2);
  assert(the_mesh.readyEstimate == 444);
  for (unsigned length : radio_driver.lengths) assert(length == 8);
  assert(the_mesh.creates == 0);
  noNativeSend();
}

static void nativeRandomBytes() {
  reset();
  MeshCoreDiscovery adapter;
  const uint32_t word = adapter.randomWord();
  assert(the_mesh.rng.calls == 1 && the_mesh.rng.size == sizeof(word));
  assert(!memcmp(&word, the_mesh.rng.bytes, sizeof(word)));
  // The adapter must preserve an RNG failure; the scheduler rejects zero tags.
  memset(the_mesh.rng.bytes, 0, sizeof(the_mesh.rng.bytes));
  assert(adapter.randomWord() == 0 && the_mesh.rng.calls == 2);
  assert(the_mesh.creates == 0 && the_mesh.readyCalls == 0);
  noNativeSend();
}

static void queuesNativeControlOnce() {
  reset();
  MeshCoreDiscovery adapter;
  assert(adapter.queueDiscovery(request, sizeof(request)));
  assert(the_mesh.readyCalls == 1 && the_mesh.readyEstimate == 137);
  assert(the_mesh.creates == 1 && the_mesh.zeroHops == 1 && the_mesh.floods == 0);
  assert(the_mesh.tracks == 1 && the_mesh.tracked == &the_mesh.packet);
  assert(the_mesh.sent == &the_mesh.packet && the_mesh.packet.length == sizeof(request));
  assert(!memcmp(the_mesh.packet.payload, request, sizeof(request)));
  assert(the_mesh.rng.calls == 0);
  assert(radio_driver.lengths.size() == 1 && radio_driver.lengths[0] == 8);
  // A successful enqueue cannot fabricate a responder or a receive ACK.
  assert(meshis::runtimeTelemetry().repeaterCount() == 0);
  assert(!meshis::activeDiscovery().active());
  assert(!meshis::activeDiscovery().receiveAllowed(0x78563412, 0));
}

static void rejectsBeforeAllocation() {
  MeshCoreDiscovery adapter;
  reset();assert(!adapter.queueDiscovery(nullptr,6));noNativeSend();
  for (unsigned length = 0; length < sizeof(request); ++length) {
    reset();
    const std::vector<uint8_t> shortRequest(request, request + length);
    assert(!adapter.queueDiscovery(shortRequest.data(), length));
    assert(the_mesh.creates == 0 && the_mesh.readyCalls == 0);
    noNativeSend();
  }
  reset();
  const uint8_t oversized[7] = {0x80, 0x04};
  assert(!adapter.queueDiscovery(oversized, sizeof(oversized)));
  assert(the_mesh.creates == 0 && the_mesh.readyCalls == 0);
  noNativeSend();

  for (unsigned scenario = 0; scenario < 3; ++scenario) {
    reset();
    uint8_t invalid[sizeof(request)];
    memcpy(invalid, request, sizeof(request));
    if (scenario == 0) invalid[0] = 0x81; // Prefix request is not our exact scan.
    if (scenario == 1) invalid[0] = 0x92; // A response is not a request.
    if (scenario == 2) invalid[1] = 0xFF; // Do not broaden the type filter.
    assert(!adapter.queueDiscovery(invalid, sizeof(invalid)));
    assert(the_mesh.creates == 0 && the_mesh.readyCalls == 0);
    noNativeSend();
  }
  reset();
  the_mesh.ready = false;
  assert(!adapter.queueDiscovery(request, sizeof(request)));
  assert(the_mesh.readyCalls == 1 && the_mesh.creates == 0);
  noNativeSend();

  reset();
  the_mesh.allocation = false;
  assert(!adapter.queueDiscovery(request, sizeof(request)));
  assert(the_mesh.readyCalls == 1 && the_mesh.creates == 1);
  noNativeSend();

  // Native sendZeroHop is void: tracking must confirm actual queue admission.
  reset();
  the_mesh.admitted = false;
  assert(!adapter.queueDiscovery(request, sizeof(request)));
  assert(the_mesh.creates == 1 && the_mesh.zeroHops == 1 && !the_mesh.floods);
  assert(the_mesh.tracks == 1 && !the_mesh.tracked);
  assert(meshis::runtimeTelemetry().repeaterCount() == 0);
  assert(!meshis::activeDiscovery().active());
}

static void localLockAlwaysBlocks() {
  MeshCoreDiscovery adapter;
  for (unsigned phase = 0; phase < 4; ++phase) {
    reset();
    auto& lock = meshis::keyLock();
    if (phase == 0) lock.setInputHeld(true);
    else {
      lock.menuPress(100); // Pending input is already blocked.
      if (phase >= 2) { lock.menuPress(200); lock.menuPress(300); }
      if (phase == 3) lock.poll(750); // Fully locked after closing animation.
    }
    assert(lock.blocks());
    assert(!adapter.queueDiscovery(request, sizeof(request)));
    assert(the_mesh.readyCalls == 0 && the_mesh.creates == 0);
    noNativeSend();
  }
}

int main() {
  savedSettingsEstimateAndReadiness();
  nativeRandomBytes();
  queuesNativeControlOnce();
  rejectsBeforeAllocation();
  localLockAlwaysBlocks();
  puts("PASS: actual MeshCoreDiscovery adapter; 8-byte airtime estimate, saved "
       "settings, native RNG/readiness, exact control request, queue failures and "
       "local lock. One native zero-hop enqueue; fake APIs only, no response ACK.");
}
