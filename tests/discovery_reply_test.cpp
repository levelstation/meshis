#include <assert.h>
#include <array>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "DiscoveryReply.h"

using Key = std::array<uint8_t, 32>;
using Reply = std::array<uint8_t, 38>;
using Hash = std::array<uint8_t, 16>;

// The shared parser and scan run unchanged. Only the enqueue boundary is fake.
struct FakeDiscovery {
  uint32_t word = 0x78563412;
  unsigned sends = 0;
  uint8_t request[6] = {};
  float frequencyMHz() const { return 869.5f; }
  float bandwidthKHz() const { return 62.5f; }
  uint32_t estimateRequestMs() const { return 100; }
  bool canQueue() const { return true; }
  uint32_t randomWord() const { return word; }
  bool queueDiscovery(const uint8_t* bytes, unsigned length) {
    assert(length == sizeof(request));
    memcpy(request, bytes, length);
    ++sends;
    return true;
  }
};

static Key key(uint8_t value) {
  Key result;
  result.fill(value);
  return result;
}

static Reply replyFor(const Key& identity, uint32_t tag) {
  Reply reply = {};
  reply[0] = 0x92;
  reply[1] = 0xB0; // Remote request SNR is -20 dB; it is not a local RSSI.
  for (unsigned n = 0; n < 4; ++n) reply[2 + n] = uint8_t(tag >> (8 * n));
  memcpy(reply.data() + 6, identity.data(), identity.size());
  return reply;
}

struct Fixture {
  FakeDiscovery radio;
  uint32_t now = 1000;
  Key self = key(0xFE);
  Hash hash = {0x10, 0x20, 0x30};
  uint8_t packet = 0;

  Fixture() {
    meshis::runtimeTelemetry() = meshis::RuntimeTelemetry{};
    meshis::activeDiscovery() = meshis::DiscoveryScan{};
    meshis::activeDiscovery().start(now);
    meshis::activeDiscovery().poll(radio, now, true, false);
    const uint8_t expected[] = {0x80, 0x04, 0x12, 0x34, 0x56, 0x78};
    assert(radio.sends == 1 && !memcmp(radio.request, expected, sizeof(expected)));
  }

  bool accept(const Reply& reply, const char* name = nullptr, bool zeroHop = true) {
    return meshis::acceptDiscoveryReply(reply.data(), reply.size(), zeroHop,
      self.data(), name, &packet, hash.data(), hash.size(), now);
  }

  void stamp(float rssi, uint32_t at) {
    meshis::runtimeTelemetry().notePacket(&packet, hash.data(), hash.size(), rssi, at);
  }

  void nextRequest() {
    const unsigned wait = meshis::activeDiscovery().secondsUntil(now);
    assert(wait <= 35);
    now += wait * 1000;
    assert(uint32_t(now - 1000) >= meshis::DiscoveryScan::minimumIntervalMs);
    ++radio.word;
    meshis::activeDiscovery().poll(radio, now, true, false);
    assert(radio.sends == 2);
    assert(meshis::activeDiscovery().requestTag() == radio.word);
  }
};

static const meshis::HeardRepeater& find(const Key& identity, uint32_t now) {
  const auto& telemetry = meshis::runtimeTelemetry();
  for (unsigned n = 0; n < telemetry.repeaterCount(); ++n) {
    const auto node = telemetry.repeater(n, now);
    if (!memcmp(node->key, identity.data(), identity.size())) return *node;
  }
  assert(false && "Expected exact full-key node");
  return *telemetry.repeater(0, now);
}

static void fullReplyNamesAndCounts() {
  Fixture f;
  const auto identity = key(0x11);
  auto reply = replyFor(identity, f.radio.word);
  f.stamp(-87.4f, f.now);
  f.now += 3;
  assert(f.accept(reply));
  auto node = find(identity, f.now);
  assert(meshis::runtimeTelemetry().repeaterCount() == 1);
  assert(!node.name[0] && node.replies == 1 && node.adverts == 0);
  assert(node.rssi == -87 && node.hasDirect && node.lastWasDirect);
  assert(node.rssiFromDiscovery && node.directSeen == 1000 && node.seen == 1003);
  assert(node.replyTag == f.radio.word);

  // Repeated radio reception cannot refresh signal strength or inflate counts.
  f.stamp(-42, f.now);
  assert(!f.accept(reply, "Duplicate must not replace name"));
  node = find(identity, f.now);
  assert(node.replies == 1 && node.adverts == 0 && node.rssi == -87 && !node.name[0]);

  const auto oldReply = reply;
  f.nextRequest();
  assert(!f.accept(oldReply));
  reply = replyFor(identity, f.radio.word);
  f.stamp(-65, f.now);
  assert(f.accept(reply, "Cached repeater name"));
  node = find(identity, f.now);
  assert(!strcmp(node.name, "Cached repeater name"));
  assert(node.replies == 2 && node.adverts == 0 && node.rssi == -65);

  // A subsequent signed advert is counted separately and owns its RSSI source.
  ++f.now;
  f.stamp(-70, f.now);
  auto& telemetry = meshis::runtimeTelemetry();
  telemetry.beginAdvert(&f.packet, f.hash.data(), f.hash.size(), true, f.now);
  telemetry.observeValidatedAdvert(identity.data(), "Advert name", true, f.now);
  telemetry.endAdvert();
  node = find(identity, f.now);
  assert(node.adverts == 1 && node.replies == 2 && node.rssi == -70);
  assert(!node.rssiFromDiscovery && !strcmp(node.name, "Advert name"));
}

static void rejectUnassociatedOrMalformedReplies() {
  Fixture f;
  const auto reply = replyFor(key(0x22), f.radio.word);
  assert(!meshis::acceptDiscoveryReply(nullptr, 38, true, f.self.data(), nullptr,
    &f.packet, f.hash.data(), f.hash.size(), f.now));
  // Exact-size allocations let ASAN detect an accidental read past a short frame.
  for (unsigned length = 0; length < reply.size(); ++length) {
    const std::vector<uint8_t> shortReply(reply.begin(), reply.begin() + length);
    assert(!meshis::acceptDiscoveryReply(shortReply.data(), length, true,
      f.self.data(), nullptr, &f.packet, f.hash.data(), f.hash.size(), f.now));
  }
  const std::array<uint8_t, 39> oversized = {};
  assert(!meshis::acceptDiscoveryReply(oversized.data(), oversized.size(), true,
    f.self.data(), nullptr, &f.packet, f.hash.data(), f.hash.size(), f.now));
  for (uint8_t flags : {0x90, 0x91, 0x93, 0x94, 0x82, 0x12, 0xF2}) {
    auto invalid = reply;
    invalid[0] = flags;
    assert(!f.accept(invalid));
  }
  assert(!f.accept(reply, nullptr, false)); // Forwarded/non-zero-hop route.
  assert(!f.accept(replyFor(f.self, f.radio.word)));
  assert(!f.accept(replyFor(key(0x22), f.radio.word + 1)));
  assert(meshis::runtimeTelemetry().repeaterCount() == 0);

  f.now += meshis::DiscoveryScan::responseMs;
  assert(!f.accept(reply)); // The exact expiration boundary must reject.
  assert(meshis::runtimeTelemetry().repeaterCount() == 0);

  f.nextRequest();
  const auto current = replyFor(key(0x22), f.radio.word);
  meshis::activeDiscovery().poll(f.radio, f.now, true, true);
  assert(!f.accept(current)); // Lock/pause invalidates the pending response.
  meshis::activeDiscovery().stop();
  assert(!f.accept(current));
  assert(meshis::runtimeTelemetry().repeaterCount() == 0);
}

static void metadataAssociationAndMissingValues() {
  for (unsigned scenario = 0; scenario < 7; ++scenario) {
    Fixture f;
    const auto identity = key(uint8_t(0x30 + scenario));
    const auto reply = replyFor(identity, f.radio.word);
    uint8_t otherPacket = 0;
    auto otherHash = f.hash;
    ++otherHash[0];
    switch (scenario) {
      case 0: break; // No metadata at all.
      case 1: meshis::runtimeTelemetry().notePacket(&otherPacket,
        f.hash.data(), f.hash.size(), -48, f.now); break;
      case 2: meshis::runtimeTelemetry().notePacket(&f.packet,
        otherHash.data(), otherHash.size(), -49, f.now); break;
      case 3: f.stamp(-50, f.now); f.now += 10001; break; // Stale sample.
      case 4: f.stamp(0, f.now); break; // Invalid driver value is unknown.
      case 5: f.stamp(-165, f.now); break;
      case 6: meshis::runtimeTelemetry().notePacket(&f.packet,
        f.hash.data(), f.hash.size() - 1, -51, f.now); break;
    }
    assert(f.accept(reply));
    const auto& node = find(identity, f.now);
    assert(node.replies == 1 && !node.adverts && !node.name[0]);
    assert(node.rssi == meshis::RuntimeTelemetry::missing);
    assert(!node.hasDirect && !node.rssiFromDiscovery && node.lastWasDirect);
  }
}

static void fullKeysStaySeparateAndNamesRemainBounded() {
  Fixture f;
  auto first = key(0x44), second = first;
  second.back() = 0x99; // Identical 8-byte prefix, distinct full identity.
  char longName[64];
  memset(longName, 'N', sizeof(longName));
  longName[sizeof(longName) - 1] = 0;
  assert(f.accept(replyFor(first, f.radio.word), longName));
  assert(f.accept(replyFor(second, f.radio.word), "Other repeater"));
  assert(meshis::runtimeTelemetry().repeaterCount() == 2);
  const auto& a = find(first, f.now);
  const auto& b = find(second, f.now);
  assert(a.replies == 1 && b.replies == 1 && !a.adverts && !b.adverts);
  assert(strlen(a.name) == 31 && a.name[31] == 0);
  assert(!strcmp(b.name, "Other repeater"));
  assert(meshis::runtimeTelemetry().shortIdCollision(a));
  assert(meshis::runtimeTelemetry().shortIdCollision(b));
}

static void missingRssiCounterIsStillAReply() {
 Fixture f;auto response=replyFor(key(0x31),f.radio.word);assert(f.accept(response));
 struct Display {
  char row[48]={};void setColor(int){}void setTextSize(int){}void setCursor(int,int){}void print(const char*){}
  void drawTextEllipsized(int,int y,int,const char* text){if(y==45)snprintf(row,sizeof(row),"%s",text);}
  void drawRect(int,int,int,int){}void fillRect(int,int,int,int){}
 } display;
 meshis::renderRepeaters(display,meshis::runtimeTelemetry(),f.now,0,true);
 assert(strstr(display.row,"PING?")&&strstr(display.row,"P:1")&&!strstr(display.row,"ADV"));
}

int main() {
  missingRssiCounterIsStillAReply();
  fullReplyNamesAndCounts();
  rejectUnassociatedOrMalformedReplies();
  metadataAssociationAndMissingValues();
  fullKeysStaySeparateAndNamesRemainBounded();
  puts("PASS: actual shared discovery reply parser; full-key/tag/route/window guards, "
       "separate reply/advert counts, exact RSSI association, unknown/cached names, "
       "duplicates and prefix collisions. Fake enqueue only.");
}
