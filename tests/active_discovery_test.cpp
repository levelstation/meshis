#include <assert.h>
#include <initializer_list>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ActiveDiscovery.h"

struct FakeDiscovery {
  float frequency = 869.618f, bandwidth = 62.5f;
  uint32_t estimate = 280, word = 5001;
  bool available = true, succeeds = true;
  unsigned attempts = 0, sends = 0, randomCalls = 0;
  uint8_t request[6] = {};
  uint32_t estimateRequestMs() const { return estimate; }
  float frequencyMHz() const { return frequency; }
  float bandwidthKHz() const { return bandwidth; }
  bool canQueue() const { return available; }
  uint32_t randomWord() { ++randomCalls; return word; }
  bool queueDiscovery(const uint8_t* bytes, unsigned size) {
    ++attempts;
    assert(size == sizeof(request));
    if (!succeeds) return false;
    memcpy(request, bytes, size);
    ++sends;
    return true;
  }
};

static void requestAndResponse() {
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  radio.word = 0x78563412;
  scan.poll(radio, 0, true, false);
  assert(!radio.sends && !scan.active());
  assert(!strcmp(scan.stateLabel(), "PASSIV"));
  scan.start(10);
  assert(!scan.receiveAllowed(radio.word, 10));
  scan.poll(radio, 10, true, false);
  const uint8_t expected[] = {0x80, 0x04, 0x12, 0x34, 0x56, 0x78};
  assert(radio.sends == 1 && !memcmp(radio.request, expected, sizeof(expected)));
  assert(scan.queuedCount() == 1 && scan.requestTag() == radio.word);
  assert(!strcmp(scan.stateLabel(), "SCAN"));
  assert(scan.receiveAllowed(radio.word, 10));
  assert(scan.receiveAllowed(radio.word, 20009));
  assert(!scan.receiveAllowed(radio.word, 20010));
  assert(!scan.receiveAllowed(0, 10));
  assert(!scan.receiveAllowed(radio.word + 1, 10));
  scan.poll(radio, 20010, true, false);
  assert(scan.requestTag() == 0 && !strcmp(scan.stateLabel(), "WARTE"));
}

static void cadenceAndSession() {
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  radio.word = 1; // 1 ms positive jitter after the first request.
  scan.start(0);
  scan.poll(radio, 0, true, false);
  assert(scan.secondsUntil(0) == 31);
  radio.word = 5000; // Maximum 5000 ms jitter after the second request.
  scan.poll(radio, 29999, true, false);
  scan.poll(radio, 30000, true, false);
  assert(radio.sends == 1);
  scan.poll(radio, 30001, true, false);
  assert(radio.sends == 2 && scan.secondsUntil(30001) == 35);
  radio.word = 5001;
  scan.poll(radio, 65000, true, false);
  assert(radio.sends == 2);
  scan.poll(radio, 65001, true, false);
  assert(radio.sends == 3);
  radio.word = 10002;
  scan.poll(radio, 95001, true, false);
  assert(radio.sends == 4);
  radio.word = 15003;
  scan.start(110000); // Repeated start cannot extend the running session.
  scan.poll(radio, 119999, true, false);
  assert(radio.sends == 4 && scan.active());
  scan.poll(radio, 120000, true, false);
  assert(!scan.active() && !strcmp(scan.stateLabel(), "ENDE"));
  assert(!scan.receiveAllowed(10002, 120000));

  meshis::DiscoveryScan late;
  FakeDiscovery second;
  late.start(0);
  late.poll(second, 0, true, false);
  second.word = 10002;
  late.poll(second, 110000, true, false);
  assert(second.sends == 2); // A late poll cannot catch up missed scans.
  late.poll(second, 110000, true, false);
  assert(second.sends == 2 && late.secondsUntil(110000) == 30);
}

static void stopLockAndReopen() {
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  scan.start(0);
  scan.poll(radio, 0, true, false);
  scan.stop();
  assert(!scan.receiveAllowed(5001, 1));
  scan.start(1000);
  scan.poll(radio, 1000, true, false);
  assert(radio.sends == 1 && scan.secondsUntil(1000) == 29);
  radio.word = 10002;
  scan.poll(radio, 30000, true, false);
  assert(radio.sends == 2);
  scan.poll(radio, 30001, true, true);
  assert(scan.requestTag() == 0 && !strcmp(scan.stateLabel(), "PAUSE"));
  assert(!scan.receiveAllowed(10002, 30002));
  scan.poll(radio, 30002, true, false);
  assert(!scan.receiveAllowed(10002, 30002));
  radio.word = 15003;
  scan.poll(radio, 60000, true, true);
  assert(radio.sends == 2);
  scan.poll(radio, 60000, true, false);
  assert(radio.sends == 2 && scan.secondsUntil(60000) == 30);
  scan.poll(radio, 90000, true, false);
  assert(radio.sends == 3);
  scan.poll(radio, 90001, false, false);
  assert(!scan.active() && scan.requestTag() == 0);
  scan.start(90002);
  radio.word = 20004;
  scan.poll(radio, 90002, true, false);
  assert(radio.sends == 3 && scan.secondsUntil(90002) == 30);
}

static void unlockStartsFreshCadence() {
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  scan.start(0);
  scan.poll(radio, 0, true, false);
  scan.poll(radio, 1000, true, true);
  scan.poll(radio, 40000, true, true);
  radio.word = 10002;
  scan.poll(radio, 40000, true, false);
  assert(radio.sends == 1 && scan.requestTag() == 0 && scan.secondsUntil(40000) == 30);
  assert(!scan.receiveAllowed(5001, 40000));
  scan.poll(radio, 69999, true, false);
  assert(radio.sends == 1);
  scan.poll(radio, 70000, true, false);
  assert(radio.sends == 2);
  scan.poll(radio, 120000, true, false);
  assert(!scan.active()); // Unlock did not reset the original session deadline.

  meshis::DiscoveryScan lowerBand;
  FakeDiscovery narrow;
  narrow.frequency = 868.95f;
  lowerBand.start(0);
  lowerBand.poll(narrow, 0, true, false);
  lowerBand.poll(narrow, 1, true, true);
  lowerBand.poll(narrow, 1000, true, false);
  assert(narrow.sends == 1 && lowerBand.secondsUntil(1000) == 1400);
}

static void failureAndRandomness() {
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  radio.available = false;
  scan.start(0);
  scan.poll(radio, 0, true, false);
  assert(!radio.attempts && !radio.randomCalls);
  assert(!strcmp(scan.stateLabel(), "FUNK BELEGT"));
  radio.available = true;
  scan.poll(radio, 4999, true, false);
  assert(!radio.attempts && !strcmp(scan.stateLabel(), "FUNK BELEGT"));
  radio.succeeds = false;
  scan.poll(radio, 5000, true, false);
  assert(radio.attempts == 1 && !scan.queuedCount() && !scan.requestTag());
  radio.succeeds = true;
  radio.word = 0;
  scan.poll(radio, 10000, true, false);
  assert(radio.attempts == 1 && !scan.requestTag());
  radio.word = 5001;
  scan.poll(radio, 14999, true, false);
  assert(radio.attempts == 1);
  scan.poll(radio, 15000, true, false);
  assert(radio.sends == 1 && scan.requestTag() == 5001);
  scan.stop();
  scan.start(45000);
  scan.poll(radio, 45000, true, false); // Repeat tag, including across stop.
  assert(radio.sends == 1 && !scan.requestTag());
  radio.word = 10002;
  scan.poll(radio, 49999, true, false);
  assert(radio.sends == 1);
  scan.poll(radio, 50000, true, false);
  assert(radio.sends == 2 && scan.requestTag() == 10002);
}

static void regionsAndBounds() {
  struct Case { float frequency, bandwidth; bool valid; };
  const Case cases[] = {
    {869.618f, 62.5f, true}, {869.525f, 250.0f, true},
    {868.3f, 600.0f, true}, {868.95f, 500.0f, true},
    {869.65f, 62.5f, false}, {869.4f, 62.5f, false},
    {868.0f, 62.5f, false}, {868.6f, 62.5f, false},
    {868.7f, 62.5f, false}, {869.2f, 62.5f, false},
    {868.65f, 62.5f, false}, {915.0f, 62.5f, false},
    {869.525f, 500.0f, false}, {868.3f, 601.0f, false},
    {869.618f, 0.0f, false}, {869.618f, -62.5f, false},
    {0.0f, 62.5f, false}, {NAN, 62.5f, false},
    {INFINITY, 62.5f, false}, {869.618f, NAN, false},
    {869.618f, INFINITY, false}
  };
  for (const auto& entry : cases) {
    meshis::DiscoveryScan scan;
    FakeDiscovery radio;
    radio.frequency = entry.frequency;
    radio.bandwidth = entry.bandwidth;
    scan.start(0);
    scan.poll(radio, 0, true, false);
    assert(bool(radio.sends) == entry.valid);
    if (!entry.valid) assert(!strcmp(scan.stateLabel(), "REGION?"));
  }
  for (uint32_t invalid : {uint32_t(0), uint32_t(20000), UINT32_MAX}) {
    meshis::DiscoveryScan scan;
    FakeDiscovery radio;
    radio.estimate = invalid;
    scan.start(0);
    scan.poll(radio, 0, true, false);
    assert(!radio.sends && !strcmp(scan.stateLabel(), "AIRTIME"));
  }
}

static void sharedAirtimeAndRollingBuckets() {
  // ceil(1.5 * 280) + 10 = 430 ms reservation.
  for (uint32_t other : {uint32_t(179570), uint32_t(179571)}) {
    meshis::DiscoveryScan scan;
    FakeDiscovery radio;
    scan.accountAirtime(0, other);
    scan.start(0);
    scan.poll(radio, 0, true, false);
    assert(radio.sends == unsigned(other == 179570));
  }
  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  scan.accountAirtime(0, 180000);
  scan.stop();
  scan.start(3600000);
  scan.poll(radio, 3600000, true, false);
  assert(!radio.sends && !strcmp(scan.stateLabel(), "AIRTIME"));
  scan.stop();
  scan.start(3659999);
  scan.poll(radio, 3659999, true, false);
  assert(!radio.sends);
  scan.poll(radio, 3665000, true, false);
  assert(radio.sends == 1); // Full 61 buckets elapsed; history expired.

  meshis::DiscoveryScan reserved;
  FakeDiscovery ownTraffic;
  reserved.accountAirtime(0, 180000 - 860);
  reserved.start(0);
  reserved.poll(ownTraffic, 0, true, false);
  ownTraffic.word = 10002;
  reserved.poll(ownTraffic, 30000, true, false);
  assert(ownTraffic.sends == 2);
  ownTraffic.word = 15003;
  reserved.poll(ownTraffic, 60000, true, false);
  reserved.poll(ownTraffic, 60001, true, false);
  assert(ownTraffic.sends == 2 && !strcmp(reserved.stateLabel(), "AIRTIME"));

  meshis::DiscoveryScan saturated;
  saturated.accountAirtime(0, UINT32_MAX);
  saturated.accountAirtime(0, UINT32_MAX);
  saturated.start(0);
  saturated.poll(radio, 0, true, false);
  assert(!saturated.queuedCount());

  for (const auto& band : {868.3f, 868.95f}) {
    meshis::DiscoveryScan limited;
    FakeDiscovery narrow;
    narrow.frequency = band;
    const uint32_t cap = band < 868.6f ? 18000 : 1800;
    limited.accountAirtime(0, cap - 429);
    limited.start(0);
    limited.poll(narrow, 0, true, false);
    assert(!narrow.sends && !strcmp(limited.stateLabel(), "AIRTIME"));
  }
}

static void adaptiveSpacingAndWrap() {
  for (const auto& band : {868.3f, 868.95f}) {
    meshis::DiscoveryScan limited;
    FakeDiscovery narrow;
    narrow.frequency = band;
    const uint32_t interval = band < 868.6f ? 140000 : 1400000;
    limited.start(0);
    limited.poll(narrow, 0, true, false);
    assert(narrow.sends == 1 && limited.secondsUntil(0) == interval / 1000);
    limited.stop();
    limited.start(interval - 1);
    narrow.word = 10002;
    limited.poll(narrow, interval - 1, true, false);
    assert(narrow.sends == 1);
    limited.poll(narrow, interval, true, false);
    assert(narrow.sends == 2);
  }

  meshis::DiscoveryScan scan;
  FakeDiscovery radio;
  radio.estimate = 1000;
  scan.start(0);
  scan.poll(radio, 0, true, false);
  assert(scan.secondsUntil(0) == 100);
  radio.word = 10002;
  scan.poll(radio, 99999, true, false);
  assert(radio.sends == 1);
  scan.poll(radio, 100000, true, false);
  assert(radio.sends == 2);

  meshis::DiscoveryScan changed;
  FakeDiscovery second;
  changed.start(0);
  changed.poll(second, 0, true, false);
  second.frequency = 868.95f; // 0.02% scan spacing: 280 * 5000 = 1400000.
  second.word = 10002;
  changed.poll(second, 30000, true, false);
  assert(second.sends == 1 && changed.secondsUntil(30000) == 1370);
  changed.stop();
  changed.start(1300000);
  changed.poll(second, 1300000, true, false);
  assert(second.sends == 1);
  changed.poll(second, 1400000, true, false);
  assert(second.sends == 2);

  meshis::DiscoveryScan wrapped;
  FakeDiscovery third;
  const uint32_t begin = UINT32_MAX - 10000;
  wrapped.start(begin);
  wrapped.poll(third, begin, true, false);
  assert(wrapped.receiveAllowed(5001, begin + 19999));
  assert(!wrapped.receiveAllowed(5001, begin + 20000));
  third.word = 10002;
  wrapped.poll(third, begin + 29999, true, false);
  assert(third.sends == 1);
  wrapped.poll(third, begin + 30000, true, false);
  assert(third.sends == 2);
  wrapped.poll(third, begin + 120000, true, false);
  assert(!wrapped.active());

  meshis::DiscoveryScan wrappedBudget;
  FakeDiscovery fourth;
  wrappedBudget.accountAirtime(begin, 180000);
  wrappedBudget.start(begin + 3600000);
  wrappedBudget.poll(fourth, begin + 3600000, true, false);
  assert(!fourth.sends);
  wrappedBudget.stop();
  wrappedBudget.start(begin + 3660000);
  wrappedBudget.poll(fourth, begin + 3660000, true, false);
  assert(fourth.sends == 1);
}

int main() {
  requestAndResponse();
  cadenceAndSession();
  stopLockAndReopen();
  unlockStartsFreshCadence();
  failureAndRandomness();
  regionsAndBounds();
  sharedAirtimeAndRollingBuckets();
  adaptiveSpacingAndWrap();
  printf("PASS: bounded native discovery scheduler (%zu bytes); exact request, tag windows, cadence/jitter, lock/reopen, region/airtime guards, shared rolling budgets, failures and millis wrap. Fake transport only.\n", sizeof(meshis::DiscoveryScan));
}
