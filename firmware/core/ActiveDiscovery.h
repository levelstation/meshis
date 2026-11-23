#pragma once

#include <stdint.h>

namespace meshis {

// An optional UI scan, never a background service. This guard observes traffic
// from its hooks; it does not replace the radio's duty-cycle enforcement or
// establish regulatory conformity. All time comparisons tolerate millis wrap.
class DiscoveryScan {
public:
  static constexpr uint32_t sessionMs = 120000;
  static constexpr uint32_t responseMs = 20000;
  static constexpr uint32_t minimumIntervalMs = 30000;
  static constexpr uint32_t failureBackoffMs = 5000;
  static constexpr unsigned maximumRequests = 4;

private:
  enum class State : uint8_t { Passive, Scan, Wait, Airtime, Busy, Region, Pause, End };
  struct BandBudget { uint32_t capMs, spacingFactor; };
  // Keeping 61 minute buckets retains at least the preceding 60 minutes.
  // Caps intentionally use an hour, not 61 minutes, and leave headroom below
  // the supported bands' published duty-cycle ceilings.
  uint32_t buckets_[61] = {};
  uint32_t bucketAt_ = 0, sessionAt_ = 0, requestAt_ = 0;
  uint32_t tag_ = 0, previousTag_ = 0, lastQueuedAt_ = 0, lastJitterMs_ = 0;
  uint32_t cooldownAt_ = 0, cooldownMs_ = 0;
  uint8_t bucket_ = 0, queued_ = 0;
  State state_ = State::Passive;
  bool bucketClock_ = false, active_ = false, accepting_ = false, haveQueued_ = false, paused_ = false;

  static BandBudget budgetFor(float frequency, float bandwidth) {
    // These comparisons also reject NaN, infinity and unknown/zero settings.
    if (!(frequency > 0.0f && frequency < 1000.0f &&
          bandwidth > 0.0f && bandwidth <= 600.0f)) return {0, 0};
    const float low = frequency - bandwidth / 2000.0f;
    const float high = frequency + bandwidth / 2000.0f;
    if (low >= 869.4f && high <= 869.65f) return {180000, 100};
    if (low >= 868.0f && high <= 868.6f) return {18000, 500};
    if (low >= 868.7f && high <= 869.2f) return {1800, 5000};
    return {0, 0};
  }

  void rotateBuckets(uint32_t now) {
    if (!bucketClock_) { bucketClock_ = true; bucketAt_ = now; return; }
    const uint32_t steps = uint32_t(now - bucketAt_) / 60000;
    if (!steps) return;
    if (steps >= 61) {
      for (unsigned n = 0; n < 61; ++n) buckets_[n] = 0;
      bucket_ = 0;
    } else {
      for (uint32_t n = 0; n < steps; ++n) {
        bucket_ = uint8_t((bucket_ + 1) % 61);
        buckets_[bucket_] = 0;
      }
    }
    bucketAt_ += steps * 60000;
  }

  uint64_t observedMs() const {
    uint64_t total = 0;
    for (unsigned n = 0; n < 61; ++n) total += buckets_[n];
    return total;
  }

  uint32_t remainingMs(uint32_t now) const {
    const uint32_t elapsed = uint32_t(now - cooldownAt_);
    return elapsed < cooldownMs_ ? cooldownMs_ - elapsed : 0;
  }

  void defer(uint32_t now) {
    if (remainingMs(now) < failureBackoffMs) {
      cooldownAt_ = now;
      cooldownMs_ = failureBackoffMs;
    }
  }

  void invalidate() { tag_ = 0; accepting_ = false; }

public:
  void start(uint32_t now) {
    if (active_) return; // Repeated start events cannot extend a live session.
    active_ = true;
    queued_ = 0;
    sessionAt_ = now;
    paused_ = false;
    invalidate();
    state_ = State::Wait;
  }

  void stop() {
    active_ = false;
    paused_ = false;
    invalidate();
    state_ = State::Passive;
    // Preserve cadence and observed airtime across closing and reopening.
  }

  // Call for completed or possibly-emitted failed transmissions. A scan also
  // reserves its charge on enqueue; accounting its completion again is an
  // intentional conservative double charge in this auxiliary guard.
  void accountAirtime(uint32_t now, uint32_t milliseconds) {
    rotateBuckets(now);
    const uint64_t total = uint64_t(buckets_[bucket_]) + milliseconds;
    buckets_[bucket_] = total > UINT32_MAX ? UINT32_MAX : uint32_t(total);
  }

  template<class Adapter>
  void poll(Adapter& adapter, uint32_t now, bool visible, bool blocked) {
    rotateBuckets(now);
    if (!active_) return;
    if (!visible) { stop(); return; }
    if (uint32_t(now - sessionAt_) >= sessionMs) {
      active_ = false;
      invalidate();
      state_ = State::End;
      return;
    }
    if (blocked) { paused_ = true; invalidate(); state_ = State::Pause; return; }
    accepting_ = true;
    if (tag_ && uint32_t(now - requestAt_) >= responseMs) tag_ = 0;

    const BandBudget budget = budgetFor(adapter.frequencyMHz(), adapter.bandwidthKHz());
    if (!budget.capMs) { invalidate(); state_ = State::Region; return; }
    const uint32_t estimate = adapter.estimateRequestMs();
    // A request that itself consumes the entire response window is not a
    // useful scan. Reject it, unknown estimates, and unsafe interval sizes.
    const uint64_t spacing = uint64_t(estimate) * budget.spacingFactor;
    if (!estimate || estimate >= responseMs || spacing > uint64_t(INT32_MAX) - 5000) {
      invalidate(); state_ = State::Airtime; return;
    }
    const uint32_t interval = spacing < minimumIntervalMs ? minimumIntervalMs : uint32_t(spacing);
    // Changing the radio settings cannot evade the previous request's cadence.
    if (haveQueued_) {
      const uint32_t elapsed = uint32_t(now - lastQueuedAt_);
      const uint32_t required = interval + lastJitterMs_;
      const uint32_t left = elapsed < required ? required - elapsed : 0;
      if (left > remainingMs(now)) { cooldownAt_ = now; cooldownMs_ = left; }
    }
    // Completing a lock or held-input gesture never triggers an overdue send.
    // Resume with a fresh interval, preserving an existing longer cooldown.
    if (paused_) {
      paused_ = false;
      const uint32_t fresh = interval + lastJitterMs_;
      if (fresh > remainingMs(now)) { cooldownAt_ = now; cooldownMs_ = fresh; }
    }
    if (queued_ >= maximumRequests || remainingMs(now)) {
      if (tag_) state_ = State::Scan;
      else if (state_ != State::Airtime && state_ != State::Busy) state_ = State::Wait;
      return;
    }

    const uint32_t reservation = uint32_t((uint64_t(estimate) * 3 + 1) / 2 + 10);
    if (observedMs() + reservation > budget.capMs) {
      state_ = State::Airtime; defer(now); return;
    }
    if (!adapter.canQueue()) { state_ = State::Busy; defer(now); return; }
    const uint32_t tag = adapter.randomWord();
    // Fail closed on a failed/repeating RNG; never manufacture a fixed tag.
    if (!tag || tag == previousTag_) { state_ = State::Wait; defer(now); return; }
    const uint8_t request[6] = {0x80, 0x04, uint8_t(tag), uint8_t(tag >> 8),
                               uint8_t(tag >> 16), uint8_t(tag >> 24)};
    if (!adapter.queueDiscovery(request, sizeof(request))) {
      invalidate(); state_ = State::Busy; defer(now); return;
    }
    accountAirtime(now, reservation);
    tag_ = previousTag_ = tag;
    requestAt_ = lastQueuedAt_ = cooldownAt_ = now;
    lastJitterMs_ = tag % 5001;
    cooldownMs_ = interval + lastJitterMs_;
    haveQueued_ = accepting_ = true;
    ++queued_;
    state_ = State::Scan;
  }

  bool receiveAllowed(uint32_t tag, uint32_t now) const {
    return active_ && accepting_ && tag && tag == tag_ &&
           uint32_t(now - sessionAt_) < sessionMs &&
           uint32_t(now - requestAt_) < responseMs;
  }
  bool active() const { return active_; }
  uint32_t requestTag() const { return tag_; }
  unsigned queuedCount() const { return queued_; }
  unsigned secondsUntil(uint32_t now) const { return unsigned((remainingMs(now) + 999) / 1000); }
  const char* stateLabel() const {
    switch (state_) {
      case State::Scan: return "SCAN";
      case State::Wait: return "WARTE";
      case State::Airtime: return "AIRTIME";
      case State::Busy: return "FUNK BELEGT";
      case State::Region: return "REGION?";
      case State::Pause: return "PAUSE";
      case State::End: return "ENDE";
      default: return "PASSIV";
    }
  }
};

inline DiscoveryScan& activeDiscovery() { static DiscoveryScan scan; return scan; }
static_assert(sizeof(DiscoveryScan) <= 512, "Bounded active discovery RAM");

} // namespace meshis
