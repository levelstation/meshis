#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
class Simulator {
  struct Impl;std::unique_ptr<Impl> impl_;
public:
  enum Event { Idle,Rx,Tx,Message,BleConnected,BleDisconnected,LowBattery,Charging,RadioError,Quiet,Reset,GpsMoving,GpsStationary,GpsStale,GpsNoFix,BotPartOne,BotPartTwo,MissingPublic,RestoreChannels,TxFail,TxOk,NoiseQuiet,NoiseBusy,NoiseUnknown };
  Simulator();~Simulator();
  void advance(uint32_t ms);void key(uint8_t value);void event(Event);
  void render();const uint8_t* buffer()const;
  void setUtc(uint32_t utc);uint32_t utc()const;int64_t local()const;
  void setLuxembourg(bool value);bool luxembourg()const;
  void setAnimations(bool value);bool animations()const;
  void showHome();void showSplash();void showPreview();void goToPage(int index);
  int pageIndex()const;static int pageCount();static const char* pageName(int index);
  const char* state()const;std::string status()const;unsigned simulatedSends()const;
  void seed();
  bool meshiPickerActive()const;bool mainMenuActive()const;
  bool soundEnabled()const;void setSoundEnabled(bool enabled);
  bool locked()const;int lockPhase()const;void menuLevel(bool pressed);
  void receiveSimulated(const char* channel,const char* sender,const char* text,bool authenticated=false,bool expected=false,bool correlated=false);
  void setSimChannel(unsigned slot,const char* name);void setSimNoise(int dbm);void setSimTxFailure(bool fail);
  std::string lastSimulatedText()const;std::string lastSimulatedChannel()const;
  unsigned rxCount()const;int quickPage()const;void setSimMotion(bool fix,long course,long speed,uint32_t age);

  void resetPreservingSettings();
  bool booting()const;
  unsigned textOverflows()const;
  unsigned previewCount()const;
  void stressText();
  bool cleanBrandMode()const;
  static bool meshiUi();static int meshiCount();static const char* meshiName(int pet);static const char* meshiDescription(int pet);int meshi()const;void selectMeshi(int pet);
  static bool classicUi();static int fixedBrand();static const char* appName();static const char* appSlug();
};
