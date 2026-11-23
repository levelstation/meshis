#pragma once

#include "Simulator.h"
#include "ActiveDiscovery.h"
#include "RuntimeTelemetry.h"
#include <cstring>

namespace radio_tests {

// Shared by the Wio and Heltec suites. Every request and reply stays in the
// Simulator's fake transport; the actual HomeScreen and input handlers run.
template<class Require, class Shot>
void discoverySimulation(Require require, Shot shot) {
  using meshis::activeDiscovery;
  using meshis::runtimeTelemetry;
  int repeaterPage = -1;
  for (int n = 0; n < Simulator::pageCount(); ++n)
    if (!std::strcmp(Simulator::pageName(n), "Repeater live")) repeaterPage = n;
  require(repeaterPage >= 0, "Discovery tests require the actual repeater page");

  auto press = [](Simulator& s, uint32_t held) {
    s.prgLevel(true); s.advance(25); s.advance(held - 25);
    s.prgLevel(false); s.advance(25);
  };
  auto confirm = [&](Simulator& s) {
    if (Simulator::heltecPrg()) press(s, 700);
    else { s.key(13); s.advance(1); }
  };
  auto next = [&](Simulator& s) {
    if (Simulator::heltecPrg()) { press(s, 60); s.advance(1201); }
    else { s.key(0xb6); s.advance(1); }
  };
  auto back = [&](Simulator& s) {
    if (Simulator::heltecPrg()) press(s, 1600);
    else { s.key(27); s.advance(1201); }
  };
  auto triple = [&](Simulator& s) {
    for (unsigned n = 0; n < 3; ++n) {
      if (Simulator::heltecPrg()) {
        press(s, 60);
        if (n < 2) s.advance(80);
      } else { s.key(27); s.advance(120); }
    }
  };

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage);
    s.advance(35000);
    require(!s.discoveryActive() && s.discoveryRequests() == 0 && s.simulatedSends() == 0,
            "Visiting or waiting on Repeater live stays passive until confirmation");
    confirm(s);
    require(s.discoveryActive() && s.discoveryRequests() == 1 && s.discoveryReplies() == 0,
            "Explicit confirmation starts one native request and an active session");
    shot(s, "discovery-active");
    require(s.injectDiscoveryReply(), "Synthetic full-key zero-hop discovery reply is accepted");
    require(s.discoveryReplies() == 1 && runtimeTelemetry().repeaterCount() == 1,
            "A real parsed reply creates exactly one measured repeater");
    const auto* node = runtimeTelemetry().repeater(0, 0);
    require(node && node->key[0] == 0xA1 && node->key[1] == 0xB2 && node->key[31] == 0xBF,
            "Discovery measurement retains the responder's complete public key");
    require(node && node->hasDirect && node->rssi == -79 && node->rssiFromDiscovery &&
            node->replies == 1 && node->adverts == 0,
            "Discovery is a PING measurement with reply count, never a signed advert");
    require(!s.injectDiscoveryReply() && s.discoveryReplies() == 1,
            "A repeated full-key reply within one request is counted once");
    shot(s, "discovery-ping");
    const unsigned sent = s.discoveryRequests();
    next(s);
    s.key(0xb5); s.advance(1); // Actual HomeScreen previous-row input on both targets.
    require(s.discoveryRequests() == sent && s.pageIndex() == repeaterPage,
            "Manual repeater next and previous selection never request another scan");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    s.advance(29999);
    require(s.discoveryRequests() == 1, "Discovery never repeats before thirty seconds");
    s.advance(5001);
    require(s.discoveryRequests() == 2, "The next due scan occurs within thirty to thirty-five seconds");
    s.advance(35000);
    s.advance(35000);
    require(s.discoveryRequests() == 4, "A continuously visible session contains at most four requests");
    s.advance(15000);
    require(!s.discoveryActive() && !std::strcmp(s.discoveryStatus(), "ENDE") &&
            s.discoveryRequests() == 4,
            "The scan session ends at 120 seconds without an automatic restart");
    require(s.discoveryReplies() == 0 && runtimeTelemetry().repeaterCount() == 0,
            "No-response scans never invent repeater identities or radio measurements");
    require(!s.injectDiscoveryReply(), "A reply cannot reopen an expired session");
    s.advance(120000);
    require(s.discoveryRequests() == 4, "An expired session remains passive indefinitely");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    confirm(s);
    require(!s.discoveryActive() && !activeDiscovery().requestTag(),
            "Confirming an open repeater list closes and stops its scanner");
    confirm(s);
    require(s.discoveryActive() && s.discoveryRequests() == 1,
            "Reopening the list preserves the previous request's cooldown");
    back(s);
    require(!s.discoveryActive() && s.pageIndex() == repeaterPage,
            "Back closes the child repeater list and stops discovery");
    confirm(s);
    require(s.discoveryRequests() == 1, "Back and reopen cannot bypass the minimum interval");
    s.showHome(); s.advance(60000);
    require(!s.discoveryActive() && !activeDiscovery().requestTag() && s.discoveryRequests() == 1,
            "Leaving the repeater view stops queued scanning and invalidates its tag");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    const uint32_t oldTag = activeDiscovery().requestTag();
    require(oldTag != 0, "An active scan has a nonzero request tag before lock");
    triple(s); s.advance(450);
    require(s.locked() && activeDiscovery().requestTag() == 0,
            "The lock gesture immediately invalidates the active discovery tag");
    require(!s.injectDiscoveryReply(oldTag), "Locked discovery rejects replies for its invalidated tag");
    const unsigned received = s.rxCount();
    s.event(Simulator::Rx); s.event(Simulator::DemoDirect); s.advance(40000);
    require(s.locked() && s.discoveryRequests() == 1 && s.rxCount() == received + 2 &&
            runtimeTelemetry().repeaterCount() == 1 && runtimeTelemetry().traceCount() > 0,
            "The locked device receives ordinary packets and adverts while discovery sends remain paused");
    shot(s, "discovery-locked");
    triple(s); s.advance(700);
    require(!s.locked() && s.discoveryRequests() == 1,
            "Unlocking never transmits an overdue request or catches up missed periods");
    const unsigned afterUnlock = s.discoveryRequests();
    s.advance(29999);
    require(s.discoveryRequests() == afterUnlock,
            "A resumed request starts a fresh cadence instead of replaying an overdue schedule");
    s.advance(5001);
    require(s.discoveryRequests() == afterUnlock + 1,
            "A visible unlocked scanner resumes with one request after a fresh thirty-to-thirty-five-second interval");
  }

  {
    Simulator s;
    s.setSoundEnabled(true); s.setLuxembourg(false); s.setAnimations(false);
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    require(s.discoveryActive(), "Reset test begins with an active scanner");
    const uint32_t savedUtc = s.utc();
    s.resetPreservingSettings();
    require(!s.discoveryActive() && !activeDiscovery().requestTag() &&
            s.discoveryRequests() == 0 && s.discoveryReplies() == 0 &&
            runtimeTelemetry().repeaterCount() == 0,
            "Normal simulator reset clears temporary discovery state and measurements");
    require(s.soundEnabled() && !s.luxembourg() && !s.animations() && s.utc() == savedUtc,
            "Resetting discovery preserves sound, timezone, animation preference and UTC");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(0.0f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    require(s.discoveryRequests() == 0 && !std::strcmp(s.discoveryStatus(), "REGION?"),
            "Unknown frequency disables discovery transmission");
    shot(s, "discovery-unknown-region");
    s.setSimDiscoveryRadio(869.618f, 0.0f, 280, false, false); s.advance(1);
    require(s.discoveryRequests() == 0 && !std::strcmp(s.discoveryStatus(), "REGION?"),
            "Unknown bandwidth disables discovery transmission");
    s.setSimDiscoveryRadio(869.618f, 250.0f, 280, false, false); s.advance(1);
    require(s.discoveryRequests() == 0,
            "A channel whose occupied bandwidth crosses the supported band edge cannot scan");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, true, false);
    s.goToPage(repeaterPage); confirm(s);
    require(!s.discoveryRequests() && !std::strcmp(s.discoveryStatus(), "FUNK BELEGT"),
            "Busy radio defers a scan without queueing a request");
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.advance(4999);
    require(!s.discoveryRequests(), "Busy-radio backoff does not immediately retry");
    s.advance(1);
    require(s.discoveryRequests() == 1, "A deferred scan may run after the radio is available");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    activeDiscovery().accountAirtime(0, 180000);
    s.goToPage(repeaterPage); confirm(s);
    require(!s.discoveryRequests() && !std::strcmp(s.discoveryStatus(), "AIRTIME"),
            "Other observed traffic can exhaust the shared auxiliary discovery budget");
    shot(s, "discovery-airtime");
    confirm(s); confirm(s);
    require(!s.discoveryRequests(), "Closing and reopening cannot immediately bypass airtime deferral");
    s.advance(5000);
    require(!s.discoveryRequests() && !std::strcmp(s.discoveryStatus(), "AIRTIME"),
            "Closing and reopening cannot reset the shared observed airtime guard");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    const uint32_t tag = activeDiscovery().requestTag();
    require(!s.injectDiscoveryReply(tag ^ 0x100u), "A mismatched request tag is rejected");
    require(!s.injectDiscoveryReply(tag, true), "A forwarded discovery reply is rejected");
    require(s.discoveryReplies() == 0 && runtimeTelemetry().repeaterCount() == 0,
            "Rejected control packets do not create repeaters");
    s.advance(20000);
    require(!s.injectDiscoveryReply(tag) && runtimeTelemetry().repeaterCount() == 0,
            "A stale tag is rejected at the twenty-second response boundary");
  }

  {
    Simulator s;
    s.setSimDiscoveryRadio(869.618f, 62.5f, 280, false, false);
    s.goToPage(repeaterPage); confirm(s);
    require(s.injectDiscoveryReply(0, false, true), "An unknown-name full-key responder is still measurable");
    const auto* node = runtimeTelemetry().repeater(0, 0);
    require(node && !node->name[0] && node->replies == 1 && node->adverts == 0 && node->rssiFromDiscovery,
            "Discovery leaves absent names unknown without manufacturing adverts or contacts");
    shot(s, "discovery-unknown-name");
  }
}

} // namespace radio_tests
