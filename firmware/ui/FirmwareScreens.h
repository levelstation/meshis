#pragma once
// Shared actual MeshCore/Meshigotchi screen classes. Dependencies supplied by target adapter.
#include "Branding.h"
class SplashScreen : public UIScreen {
  UITask* _task;
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
  NodePrefs* _splash_prefs;
#endif
  unsigned long dismiss_after;
  char _version_info[12];
#if defined(LU_ALERT_BRANDING) || defined(MESHIGOTCHI_GOTCHI_BOARD)
  uint32_t _logo_start = 0;
  bool _logo_started = false;
#endif

public:
  SplashScreen(UITask* task
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    ,NodePrefs* prefs=nullptr
#endif
  ) : _task(task)
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    ,_splash_prefs(prefs)
#endif
  {
    // strip off dash and commit hash by changing dash to null terminator
    // e.g: v1.2.3-abcdef -> v1.2.3
    const char *ver = FIRMWARE_VERSION;
    const char *dash = strchr(ver, '-');

    int len = dash ? dash - ver : strlen(ver);
    if (len >= sizeof(_version_info)) len = sizeof(_version_info) - 1;
    memcpy(_version_info, ver, len);
    _version_info[len] = 0;

    dismiss_after = millis() + BOOT_SCREEN_MILLIS;
  }

  int render(DisplayDriver& display) override {
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    if(!_logo_started){_logo_start=millis();_logo_started=true;}
    const uint32_t elapsed=static_cast<uint32_t>(millis()-_logo_start);
    display.setColor(1);display.setTextSize(1);display.drawTextCentered(64,0,"MESHIS");
    meshigotchi_gotchi::sprite(display,_splash_prefs?meshigotchi_gotchi::valid(_splash_prefs->meshigotchi_pet):meshigotchi_gotchi::defaultPet,48,15+(elapsed/200)%2,2);
    display.drawTextCentered(64,55,"MeshCore / Board");return 200;
#elif defined(LU_ALERT_BRANDING)
    if (!_logo_started) { _logo_start = millis(); _logo_started = true; }
    const uint32_t elapsed = static_cast<uint32_t>(millis() - _logo_start);
    meshigotchi_brand::render(display, elapsed >= 2500 ? 2500 : static_cast<uint16_t>(elapsed));
    return 100;
#else
    // meshcore logo
    display.setColor(UIColor::corp_blue);
    int logoWidth = 128;
    display.drawXbm((display.width() - logoWidth) / 2, 3, meshcore_logo, logoWidth, 13);

    // meshcore website
    const char* website = "https://meshcore.io";
    display.setColor(UIColor::primary_txt);
    display.setTextSize(1);
    uint16_t websiteWidth = display.getTextWidth(website);
    display.setCursor((display.width() - websiteWidth) / 2, 22);
    display.print(website);

    // version info
    display.setColor(UIColor::primary_txt);
    display.setTextSize(1);
    display.drawTextCentered(display.width()/2, 35, _version_info);

    display.setColor(UIColor::secondary_txt);
    display.setTextSize(1);
    display.drawTextCentered(display.width()/2, 48, FIRMWARE_BUILD_DATE);

    return 1000;
#endif
  }

  void poll() override {
#if defined(LU_ALERT_BRANDING) || defined(MESHIGOTCHI_GOTCHI_BOARD)
    if (_logo_started && static_cast<uint32_t>(millis() - _logo_start) >= 2500) {
#else
    if (millis() >= dismiss_after) {
#endif
      _task->gotoHomeScreen();
    }
  }
};

class HomeScreen : public UIScreen {
  enum HomePage {
    FIRST,
    TIMEZONE,
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    BOARD,
#endif
#if !defined(BRAND_CLASSIC_UI) || defined(MESHIGOTCHI_GOTCHI_BOARD)
    ANIMATIONS,
#endif
    DIAGNOSTICS,
    MESSAGES,
    RECENT,
    RADIO,
    BLUETOOTH,
    ADVERT,
#if ENV_INCLUDE_GPS == 1
    GPS,
#endif
#if UI_SENSORS_PAGE == 1
    SENSORS,
#endif
#if UI_DISCOVER_SCREEN
    DISCOVERY,
#endif
#ifndef UI_NO_HIBERNATE
    SHUTDOWN,
#endif
    Count    // keep as last
  };

  UITask* _task;
  mesh::RTCClock* _rtc;
  SensorManager* _sensors;
  NodePrefs* _node_prefs;
  uint8_t _page;
  bool _shutdown_init;
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
  uint8_t _gotchi_cursor=0;
#endif
#if defined(BRAND_CLASSIC_UI)
  bool _classic_menu=false;
  uint8_t _classic_selection=0;
  static HomePage menuPage(unsigned index) {
    static const HomePage order[]={
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
      BOARD,ANIMATIONS,
#endif
      MESSAGES,RECENT,RADIO,BLUETOOTH,
#if ENV_INCLUDE_GPS == 1
      GPS,
#endif
#if UI_SENSORS_PAGE == 1
      SENSORS,
#endif
      ADVERT,
#if UI_DISCOVER_SCREEN
      DISCOVERY,
#endif
      DIAGNOSTICS,TIMEZONE,
#ifndef UI_NO_HIBERNATE
      SHUTDOWN,
#endif
    };
    return order[index%(Count-1)];
  }
#endif
  AdvertPath recent[UI_RECENT_LIST_SIZE];
#if UI_DISCOVER_SCREEN
  DiscoveredNode discovered[DISCOVERED_NODES_TABLE_SIZE]; // not circular, latest discovered nodes are not kept
  uint32_t disc_node_req_tag = 0;
  uint32_t disc_nodes_count = 0;
  uint32_t discovery_req_time = 0;
  bool discovery_disp_names = true; // by default desplay names if available (removes SNR_O)
#endif

  void renderBatteryIndicator(DisplayDriver& display, uint16_t batteryMilliVolts) {
    // Convert millivolts to percentage
#ifndef BATT_MIN_MILLIVOLTS
  #define BATT_MIN_MILLIVOLTS 3000
#endif
#ifndef BATT_MAX_MILLIVOLTS
  #define BATT_MAX_MILLIVOLTS 4200
#endif
    const int minMilliVolts = BATT_MIN_MILLIVOLTS;
    const int maxMilliVolts = BATT_MAX_MILLIVOLTS;
    int batteryPercentage = ((batteryMilliVolts - minMilliVolts) * 100) / (maxMilliVolts - minMilliVolts);
    if (batteryPercentage < 0) batteryPercentage = 0; // Clamp to 0%
    if (batteryPercentage > 100) batteryPercentage = 100; // Clamp to 100%

    // battery icon
    int iconWidth = 24;
    int iconHeight = 10;
    int iconX = display.width() - iconWidth - 5; // Position the icon near the top-right corner
    int iconY = 0;
    display.setColor(UIColor::title_txt);

    // battery outline
    display.drawRect(iconX, iconY, iconWidth, iconHeight);

    // battery "cap"
    display.fillRect(iconX + iconWidth, iconY + (iconHeight / 4), 3, iconHeight / 2);

    // fill the battery based on the percentage
    int fillWidth = (batteryPercentage * (iconWidth - 4)) / 100;
    display.fillRect(iconX + 2, iconY + 2, fillWidth, iconHeight - 4);

    // while charging, show a bolt (or a plug once full) just left of the battery,
    // keeping the fill bar itself clean and uninterrupted
    bool charging = chargeStatus == meshigotchi::ChargeStatus::Charging;
    if (charging) {
      // There's no charge-complete signal on most boards, so "full" is a high
      // voltage band rather than an exact 100% (a real pack rarely reads 4.2V).
      const int BATT_FULL_PCT = 95;
      const uint8_t* symbol = (batteryPercentage >= BATT_FULL_PCT) ? plug_icon : charging_icon;
      display.setColor(UIColor::title_txt);
      display.drawXbm(iconX - 9, iconY + 1, symbol, 8, 8);
    }

    // show muted icon if buzzer is muted (shifted further left when the charging
    // icon already occupies the slot immediately left of the battery)
#ifdef PIN_BUZZER
    if (_task->isBuzzerQuiet()) {
      display.setColor(UIColor::warning_txt);
      display.drawXbm(iconX - (charging ? 18 : 9), iconY + 1, muted_icon, 8, 8);
    }
#endif
  }

  CayenneLPP sensors_lpp;
  int sensors_nb = 0;
  bool sensors_scroll = false;
  int sensors_scroll_offset = 0;
  int next_sensors_refresh = 0;

  void refresh_sensors() {
    if (millis() > next_sensors_refresh) {
      sensors_lpp.reset();
      sensors_nb = 0;
      sensors_lpp.addVoltage(TELEM_CHANNEL_SELF, (float)board.getBattMilliVolts() / 1000.0f);
      sensors.querySensors(0xFF, sensors_lpp);
      LPPReader reader (sensors_lpp.getBuffer(), sensors_lpp.getSize());
      uint8_t channel, type;
      while(reader.readHeader(channel, type)) {
        reader.skipData(type);
        sensors_nb ++;
      }
      sensors_scroll = sensors_nb > UI_RECENT_LIST_SIZE;
#if AUTO_OFF_MILLIS > 0
      next_sensors_refresh = millis() + 5000; // refresh sensor values every 5 sec
#else
      next_sensors_refresh = millis() + 60000; // refresh sensor values every 1 min
#endif
    }
  }

public:
  int pageIndex() const { return _page; }
#if defined(BRAND_CLASSIC_UI)
  bool menuActive()const{return _classic_menu;}
  void selectPage(int index){if(index>=0 && index<Count){_page=index;_classic_menu=false;
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
      _gotchi_cursor=meshigotchi_gotchi::valid(_node_prefs->meshigotchi_pet);
#endif
    }}
#endif
  static int pageCount() { return HomePage::Count; }
  static const char* pageName(int page) {
    switch(page) {
      case FIRST:return "Home";case TIMEZONE:return "Timezone";
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
      case BOARD:return "Board";
#endif
#if !defined(BRAND_CLASSIC_UI) || defined(MESHIGOTCHI_GOTCHI_BOARD)
      case ANIMATIONS:return "Animations";
#endif
      case DIAGNOSTICS:return "Diagnostics";case MESSAGES:return "Messages";case RECENT:return "Recent adverts";
      case RADIO:return "Radio";case BLUETOOTH:return "Bluetooth";case ADVERT:return "Advertise";
#if ENV_INCLUDE_GPS == 1
      case GPS:return "GPS";
#endif
#if UI_SENSORS_PAGE == 1
      case SENSORS:return "Sensors";
#endif
#if UI_DISCOVER_SCREEN
      case DISCOVERY:return "Discovery";
#endif
#ifndef UI_NO_HIBERNATE
      case SHUTDOWN:return "Hibernate";
#endif
      default:return "Unknown";
    }
  }
  HomeScreen(UITask* task, mesh::RTCClock* rtc, SensorManager* sensors, NodePrefs* node_prefs)
     : _task(task), _rtc(rtc), _sensors(sensors), _node_prefs(node_prefs), _page(0),
       _shutdown_init(false), sensors_lpp(200) {  }

#if UI_DISCOVER_SCREEN
  bool sendDiscoverRequest() {
    uint8_t cmd_bytes[6];
    cmd_bytes[0] = CTL_TYPE_NODE_DISCOVER_REQ | 1; // DISCOVER_REQ | prefix only
    cmd_bytes[1] = 0xFF;     // Repeaters
    the_mesh.getRNG()->random((uint8_t *) &disc_node_req_tag, 4);  // generate random tag
    memcpy(&cmd_bytes[2], &disc_node_req_tag, 4);
    disc_nodes_count = 0;
    mesh::Packet* req = the_mesh.createControlData(cmd_bytes, sizeof(cmd_bytes));
    if (req) {
      the_mesh.sendZeroHop(req);
      discovery_req_time = millis();
      return true;
    }
    return false;
  }

  void handleDiscoverResponse(const mesh::Packet* packet) {
    if (disc_nodes_count < DISCOVERED_NODES_TABLE_SIZE && memcmp(&packet->payload[2], &disc_node_req_tag, 4) == 0) {
      auto d = &discovered[disc_nodes_count++];
      memcpy(d->pubkey_prefix, &packet->payload[6], 8);
      d->type = packet->payload[0] & 0xF;
      d->snr_out = ((int8_t)packet->payload[1]) / 4.0;
      d->snr_in = radio_driver.getLastSNR();
      ContactInfo* c = the_mesh.lookupContactByPubKey(&packet->payload[6], 8);
      if (c != NULL) {
        snprintf(d->name,sizeof(d->name),"%.31s",c->name);
      } else {
        d->name[0] = 0;
      }
    }
  }
#endif

  void poll() override {
    if (_shutdown_init && !_task->isButtonPressed()) {  // must wait for USR button to be released
      _task->shutdown();
    }
  }

  int render(DisplayDriver& display) override {
    nuxLastFrame=millis();
#if defined(BRAND_CLASSIC_UI)
    if(_classic_menu){
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      display.drawTextEllipsized(0,0,128,meshigotchi_brand::menuLabel());
      display.fillRect(0,10,128,1);
      const unsigned first=(_classic_selection/4)*4;
      for(unsigned row=0;row<4 && first+row<Count-1;++row){
        char line[32];snprintf(line,sizeof(line),"%c %s",first+row==_classic_selection?'>':' ',pageName(menuPage(first+row)));
        display.drawTextEllipsized(0,15+row*12,128,line);
      }
      return 1000;
    }
#endif
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    if(_page==HomePage::BOARD){meshigotchi_gotchi::board(display,_gotchi_cursor,meshigotchi_gotchi::valid(_node_prefs->meshigotchi_pet));return 1000;}
    if(_page==HomePage::FIRST){
      display.setColor(1);display.setTextSize(1);display.drawTextEllipsized(0,0,84,"MESHIS");renderBatteryIndicator(display,nuxBatteryMv);
      display.fillRect(0,11,128,1);const uint8_t pet=meshigotchi_gotchi::valid(_node_prefs->meshigotchi_pet);
      meshigotchi_gotchi::draw(display,pet,nux,millis(),_node_prefs->meshigotchi_animations!=0);
      display.drawTextEllipsized(50,22,76,meshigotchi_gotchi::name(pet));
      display.drawTextEllipsized(50,34,56,meshigotchi_gotchi::stateName(nux.state(millis())));
      char info[32];if(nux.errorCode())snprintf(info,sizeof(info),"E%03u",nux.errorCode());else snprintf(info,sizeof(info),"Enter: Menu");
      display.drawTextEllipsized(50,46,76,info);
      snprintf(info,sizeof(info),"R%lu T%lu B%c",(unsigned long)radio_driver.getPacketsRecv(),(unsigned long)radio_driver.getPacketsSent(),_task->hasConnection()?'+':'-');
      display.drawTextEllipsized(0,56,128,info);return nux.interval(millis(),_node_prefs->meshigotchi_animations!=0);
    }
#endif
#if defined(LU_ALERT_BRANDING)
    if (_page == HomePage::FIRST) {
      meshigotchi_brand::render(display, 2500);
      return 1000;
    }
#endif
    display.setColor(UIColor::title_bkg);
    display.fillRect(0, 0, display.width(), 12);
    char tmp[80];
    // node name
    display.setTextSize(1);
    display.setColor(UIColor::title_txt);
    char filtered_name[sizeof(_node_prefs->node_name)];
#if defined(LU_ALERT_BRANDING) || defined(MESHIGOTCHI_GOTCHI_BOARD)
    display.translateUTF8ToBlocks(filtered_name, meshigotchi_brand::menuLabel(), sizeof(filtered_name));
#else
    display.translateUTF8ToBlocks(filtered_name, _node_prefs->node_name, sizeof(filtered_name));
#endif
    display.setCursor(0, 2);
    display.drawTextEllipsized(0,2,78,filtered_name);

    // battery voltage
    renderBatteryIndicator(display, nuxBatteryMv);

    // curr page indicator
    if (UIColor::title_bkg == UIColor::window_bkg) {
      display.setColor(UIColor::title_txt);
    } else {
      display.setColor(UIColor::title_bkg);
    }
    int y = 14;
    int x = display.width() / 2 - 5 * (HomePage::Count-1);
    for (uint8_t i = 0; i < HomePage::Count; i++, x += 10) {
      if (i == _page) {
        display.fillRect(x-1, y-1, 4, 4);
      } else {
        display.fillRect(x, y, 2, 2);
      }
    }

#if !defined(BRAND_CLASSIC_UI)
    if (_page == HomePage::FIRST) {
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      const uint32_t utc = _rtc->getCurrentTime();
      const DateTime dt(static_cast<uint32_t>(meshigotchi::localTime(utc, _node_prefs->meshigotchi_timezone == 1)));
      if (utc < 1577836800UL) snprintf(tmp, sizeof(tmp), "Time not synced");
      else snprintf(tmp, sizeof(tmp), "%02d:%02d %02d/%02d/%04d", dt.hour(), dt.minute(), dt.day(), dt.month(), dt.year());
      display.setCursor(0,20);display.print(tmp);
      meshigotchi::drawNux(display,nux,millis(),_node_prefs->meshigotchi_animations!=0);
      display.setCursor(0,36);display.print(meshigotchi::stateLabel(nux.state(millis())));
      if(nux.errorCode()) {
        snprintf(tmp,sizeof(tmp),"E%03u",nux.errorCode());display.setCursor(0,46);display.print(tmp);
      }
      if (!_task->hasConnection() && the_mesh.getBLEPin()!=0)
        snprintf(tmp,sizeof(tmp),"PIN:%06lu",(unsigned long)the_mesh.getBLEPin());
      else {
        const uint32_t rx=radio_driver.getPacketsRecv(),tx=radio_driver.getPacketsSent();
        snprintf(tmp,sizeof(tmp),"R%lu%s T%lu%s B%c",(unsigned long)(rx>99999?99999:rx),rx>99999?"+":"",
          (unsigned long)(tx>99999?99999:tx),tx>99999?"+":"",_task->hasConnection()?'+':'-');
      }
      display.setCursor(0,55);display.print(tmp);
      return nux.interval(millis(),_node_prefs->meshigotchi_animations!=0);
    }
#endif
    if (_page == HomePage::TIMEZONE) {
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      display.setCursor(0,24);display.print("Timezone / Enter");
      display.setCursor(0,40);display.print(_node_prefs->meshigotchi_timezone==1?"Europe/Luxembourg":"UTC");
#if !defined(BRAND_CLASSIC_UI) || defined(MESHIGOTCHI_GOTCHI_BOARD)
    } else if (_page == HomePage::ANIMATIONS) {
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      display.setCursor(0,24);display.print("Animations / Enter");
      display.setCursor(0,40);display.print(_node_prefs->meshigotchi_animations?"On":"Off");
#endif
    } else if (_page == HomePage::MESSAGES) {
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      if(messages.active())messages.render(display,messageMesh);
      else {display.setCursor(0,24);display.print("Messages / Enter");display.setCursor(0,40);display.print("Inbox Channels People");}
    } else if (_page == HomePage::DIAGNOSTICS) {
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      snprintf(tmp,sizeof(tmp),"RX %lu",(unsigned long)radio_driver.getPacketsRecv());display.setCursor(0,20);display.print(tmp);
      snprintf(tmp,sizeof(tmp),"TX %lu E%lu",(unsigned long)radio_driver.getPacketsSent(),(unsigned long)radio_driver.getTxFailures());
      display.setCursor(0,30);display.drawTextEllipsized(0,30,128,tmp);
      if(radio_driver.getPacketsRecv())snprintf(tmp,sizeof(tmp),"RSSI %.0f SNR %.1f",radio_driver.getLastRSSI(),radio_driver.getLastSNR());
      else snprintf(tmp,sizeof(tmp),"No packets observed");
      display.setCursor(0,40);display.drawTextEllipsized(0,40,128,tmp);
      const char* charge=chargeStatus==meshigotchi::ChargeStatus::Charging?"Charging":
        chargeStatus==meshigotchi::ChargeStatus::Full?"Charge done":
        chargeStatus==meshigotchi::ChargeStatus::Invalid?"Charge invalid":"Not charging";
      display.setCursor(0,52);display.print(charge);
    } else if (_page == HomePage::RECENT) {
      the_mesh.getRecentlyHeard(recent, UI_RECENT_LIST_SIZE);
      display.setColor(UIColor::primary_txt);
      int y = 20;
      for (int i = 0; i < UI_RECENT_LIST_SIZE; i++, y += 11) {
        auto a = &recent[i];
        if (a->name[0] == 0) continue;  // empty slot
        int secs = _rtc->getCurrentTime() - a->recv_timestamp;
        if (secs < 60) {
          sprintf(tmp, "%ds", secs);
        } else if (secs < 60*60) {
          sprintf(tmp, "%dm", secs / 60);
        } else {
          sprintf(tmp, "%dh", secs / (60*60));
        }

        int timestamp_width = display.getTextWidth(tmp);
        int max_name_width = display.width() - timestamp_width - 1;

        char filtered_recent_name[sizeof(a->name)];
        display.translateUTF8ToBlocks(filtered_recent_name, a->name, sizeof(filtered_recent_name));
        display.drawTextEllipsized(0, y, max_name_width, filtered_recent_name);
        display.setCursor(display.width() - timestamp_width - 1, y);
        display.print(tmp);
      }
    } else if (_page == HomePage::RADIO) {
      display.setColor(UIColor::primary_txt);
      display.setTextSize(1);
      // freq / sf
      display.setCursor(0, 20);
      snprintf(tmp,sizeof(tmp), "FQ:%07.3f SF:%d", _node_prefs->freq, _node_prefs->sf);
      display.drawTextEllipsized(0,20,display.width(),tmp);

      display.setCursor(0, 31);
      snprintf(tmp,sizeof(tmp), "BW:%.2f CR:%d", _node_prefs->bw, _node_prefs->cr);
      display.drawTextEllipsized(0,31,display.width(),tmp);

      // tx power,  noise floor
      display.setCursor(0, 42);
      sprintf(tmp, "TX: %ddBm", _node_prefs->tx_power_dbm);
      display.drawTextEllipsized(0,42,display.width(),tmp);
      display.setCursor(0, 53);
      sprintf(tmp, "Noise floor: %d", radio_driver.getNoiseFloor());
      display.drawTextEllipsized(0,53,display.width(),tmp);
    } else if (_page == HomePage::BLUETOOTH) {
#if defined(BRAND_CLASSIC_UI)
      display.setColor(UIColor::primary_txt);display.setTextSize(1);
      display.setCursor(0,20);display.print(_task->isBluetoothEnabled()?"Bluetooth: On":"Bluetooth: Off");
      snprintf(tmp,sizeof(tmp),"PIN: %06lu",(unsigned long)the_mesh.getBLEPin());
      display.setCursor(0,32);display.print(the_mesh.getBLEPin()?tmp:"PIN: not required");
      display.setCursor(0,44);display.print(_task->hasConnection()?"Connected":"Not connected");
      display.setCursor(0,56);display.print("Enter: On / Off");
#else
      display.setColor(UIColor::corp_blue);
      display.drawXbm((display.width() - 32) / 2, 18,
          _task->isBluetoothEnabled() ? bluetooth_on : bluetooth_off,
          32, 32);
      display.setColor(UIColor::secondary_txt);
      display.setTextSize(1);
      display.drawTextCentered(display.width() / 2, 64 - 11, "toggle: " PRESS_LABEL);
#endif
    } else if (_page == HomePage::ADVERT) {
      display.setColor(UIColor::corp_blue);
      display.drawXbm((display.width() - 32) / 2, 18, advert_icon, 32, 32);
      display.setColor(UIColor::secondary_txt);
      display.drawTextCentered(display.width() / 2, 64 - 11, "advert: " PRESS_LABEL);
#if ENV_INCLUDE_GPS == 1
    } else if (_page == HomePage::GPS) {
      LocationProvider* nmea = sensors.getLocationProvider();
      char buf[50];
      int y = 18;
      bool gps_state = _task->getGPSState();
#ifdef PIN_GPS_SWITCH
      bool hw_gps_state = digitalRead(PIN_GPS_SWITCH);
      if (gps_state != hw_gps_state) {
        strcpy(buf, gps_state ? "gps off(hw)" : "gps off(sw)");
      } else {
        strcpy(buf, gps_state ? "gps on" : "gps off");
      }
#else
      strcpy(buf, gps_state ? "gps on" : "gps off");
#endif
      display.setColor(UIColor::primary_txt);
      display.drawTextLeftAlign(0, y, buf);
      if (nmea == NULL) {
        y = y + 12;
        display.setColor(UIColor::secondary_txt);
        display.drawTextLeftAlign(0, y, "Can't access GPS");
      } else {
        display.setColor(UIColor::primary_txt);
        strcpy(buf, nmea->isValid()?"fix":"no fix");
        display.drawTextEllipsized(28,y,display.width()-28,buf);
        y = y + 12;
        display.setColor(UIColor::secondary_txt);
        display.drawTextLeftAlign(0, y, "sat");
        display.setColor(UIColor::primary_txt);
        sprintf(buf, "%d", nmea->satellitesCount());
        display.drawTextEllipsized(28,y,display.width()-28,buf);
        y = y + 12;
        display.setColor(UIColor::secondary_txt);
        display.drawTextLeftAlign(0, y, "pos");
        display.setColor(UIColor::primary_txt);
        sprintf(buf, "%.4f %.4f",
          nmea->getLatitude()/1000000., nmea->getLongitude()/1000000.);
        display.drawTextEllipsized(28,y,display.width()-28,buf);
        y = y + 12;
        display.setColor(UIColor::secondary_txt);
        display.drawTextLeftAlign(0, y, "alt");
        display.setColor(UIColor::primary_txt);
        sprintf(buf, "%.2f", nmea->getAltitude()/1000.);
        display.drawTextEllipsized(28,y,display.width()-28,buf);
        y = y + 12;
      }
#endif
#if UI_SENSORS_PAGE == 1
    } else if (_page == HomePage::SENSORS) {
      int y = 18;
      refresh_sensors();
      char buf[30];
      char name[30];
      LPPReader r(sensors_lpp.getBuffer(), sensors_lpp.getSize());

      for (int i = 0; i < sensors_scroll_offset; i++) {
        uint8_t channel, type;
        r.readHeader(channel, type);
        r.skipData(type);
      }

      for (int i = 0; i < (sensors_scroll?UI_RECENT_LIST_SIZE:sensors_nb); i++) {
        uint8_t channel, type;
        if (!r.readHeader(channel, type)) { // reached end, reset
          r.reset();
          r.readHeader(channel, type);
        }

        display.setCursor(0, y);
        float v;
        switch (type) {
          case LPP_GPS: // GPS
            float lat, lon, alt;
            r.readGPS(lat, lon, alt);
            strcpy(name, "gps"); sprintf(buf, "%.4f %.4f", lat, lon);
            break;
          case LPP_VOLTAGE:
            r.readVoltage(v);
            strcpy(name, "voltage"); sprintf(buf, "%6.2f", v);
            break;
          case LPP_CURRENT:
            r.readCurrent(v);
            strcpy(name, "current"); sprintf(buf, "%.3f", v);
            break;
          case LPP_TEMPERATURE:
            r.readTemperature(v);
            strcpy(name, "temperature"); sprintf(buf, "%.2f", v);
            break;
          case LPP_RELATIVE_HUMIDITY:
            r.readRelativeHumidity(v);
            strcpy(name, "humidity"); sprintf(buf, "%.2f", v);
            break;
          case LPP_BAROMETRIC_PRESSURE:
            r.readPressure(v);
            strcpy(name, "pressure"); sprintf(buf, "%.2f", v);
            break;
          case LPP_ALTITUDE:
            r.readAltitude(v);
            strcpy(name, "altitude"); sprintf(buf, "%.0f", v);
            break;
          case LPP_POWER:
            r.readPower(v);
            strcpy(name, "power"); sprintf(buf, "%6.2f", v);
            break;
          default:
            r.skipData(type);
            strcpy(name, "unk"); sprintf(buf, "");
        }
        display.setCursor(0, y);
        display.setColor(UIColor::secondary_txt);
        display.drawTextEllipsized(0,y,62,name);
        display.setColor(UIColor::primary_txt);
        display.setCursor(
          display.width()-display.getTextWidth(buf)-1, y
        );
        display.drawTextEllipsized(66,y,display.width()-66,buf);
        y = y + 12;
      }
      if (sensors_scroll) sensors_scroll_offset = (sensors_scroll_offset+1)%sensors_nb;
      else sensors_scroll_offset = 0;
#endif
#if UI_DISCOVER_SCREEN
    } else if (_page == HomePage::DISCOVERY) {
      int count = disc_nodes_count;
      display.setColor(UIColor::primary_txt);
      int y = 20;
      for (int i = 0; i < count; i++, y += 11) {
        char name[32];
        auto a = &discovered[i];
        if ((a->name[0] == 0) || !discovery_disp_names) {
          mesh::Utils::toHex(name, a->pubkey_prefix, 4);
        } else {
          snprintf(name,sizeof(name),"%.31s",a->name);
        }
        char filtered_name[sizeof(name)];
        char snr_s[12];
        if (strlen(name) <= 8) { // display snr_o
          sprintf(snr_s, "%02.1f>%02.1f", a->snr_out, a->snr_in);
        } else {
          sprintf(snr_s, "%02.1f", a->snr_in);
        }
        int snr_width = display.getTextWidth(snr_s);
        int max_name_width = display.width() - snr_width - 1;
        display.translateUTF8ToBlocks(filtered_name, name, sizeof(filtered_name));
        display.drawTextEllipsized(0, y, max_name_width, filtered_name);
        display.setCursor(display.width() - snr_width - 1, y);
        display.drawTextEllipsized(display.width()-snr_width-1,y,snr_width,snr_s);
      }
      if (millis() < discovery_req_time + 5000) {
        return 1000; // more frequent updates just after req
      } else if (count < DISCOVERED_NODES_TABLE_SIZE -1) { // show only 5 sec after last disc
        y = 10 + 11 * DISCOVERED_NODES_TABLE_SIZE;
        display.drawTextCentered(display.width() / 2, y, "discover: " PRESS_LABEL);
      }
#endif
#ifndef UI_NO_HIBERNATE
    } else if (_page == HomePage::SHUTDOWN) {
      display.setColor(UIColor::corp_blue);
      display.setTextSize(1);
      if (_shutdown_init) {
        display.setColor(UIColor::warning_txt);
        display.drawTextCentered(display.width() / 2, 34, "hibernating...");
      } else {
        display.setColor(UIColor::secondary_txt);
        display.drawXbm((display.width() - 32) / 2, 18, power_icon, 32, 32);
        display.drawTextCentered(display.width() / 2, 64 - 11, "hibernate:" PRESS_LABEL);
      }
#endif
    }
    return 5000;   // next render after 5000 ms
  }

  bool handleInput(char c) override {
    const uint8_t key=static_cast<uint8_t>(c);
#if defined(BRAND_CLASSIC_UI)
    if(_classic_menu){
      if(key==KEY_CANCEL){_classic_menu=false;return true;}
      if(key==KEY_UP || key==KEY_LEFT || key==KEY_PREV)_classic_selection=(_classic_selection+Count-2)%(Count-1);
      else if(key==KEY_DOWN || key==KEY_RIGHT || key==KEY_NEXT)_classic_selection=(_classic_selection+1)%(Count-1);
      else if(key==KEY_ENTER || key==KEY_SELECT){_page=menuPage(_classic_selection);_classic_menu=false;
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
        _gotchi_cursor=meshigotchi_gotchi::valid(_node_prefs->meshigotchi_pet);
#endif
      }
      return true;
    }
#endif
#if defined(MESHIGOTCHI_GOTCHI_BOARD)
    if(_page==HomePage::BOARD){
      if(key==KEY_CANCEL || key==KEY_SELECT){_page=FIRST;_classic_menu=true;}
      else if(key==KEY_LEFT || key==KEY_PREV)_gotchi_cursor=(_gotchi_cursor+meshigotchi_gotchi::count-1)%meshigotchi_gotchi::count;
      else if(key==KEY_RIGHT || key==KEY_NEXT)_gotchi_cursor=(_gotchi_cursor+1)%meshigotchi_gotchi::count;
      else if(key==KEY_UP || key==KEY_DOWN){
        const uint8_t first=(_gotchi_cursor/meshigotchi_gotchi::boardPageSize)*meshigotchi_gotchi::boardPageSize;
        const uint8_t slot=_gotchi_cursor-first,target=first+(slot<3?slot+3:slot-3);
        if(target<meshigotchi_gotchi::count)_gotchi_cursor=target;
      }
      else if(key==KEY_ENTER){_node_prefs->meshigotchi_pet=_gotchi_cursor;the_mesh.savePrefs();_page=FIRST;}
      return true;
    }
#endif
    if(_page==HomePage::MESSAGES && messages.active()) {
      if(key==KEY_LEFT || key==KEY_UP || key==KEY_PREV)messages.input(meshigotchi::Input::Previous,messageMesh);
      else if(key==KEY_RIGHT || key==KEY_DOWN || key==KEY_NEXT)messages.input(meshigotchi::Input::Next,messageMesh);
      else if(key==KEY_ENTER)messages.input(meshigotchi::Input::Enter,messageMesh);
      else if(key==KEY_CANCEL || key==KEY_SELECT)messages.input(meshigotchi::Input::Back,messageMesh);
      return true;
    }
#if defined(BRAND_CLASSIC_UI)
    if(key==KEY_CANCEL){if(_page!=HomePage::FIRST)_classic_menu=true;_page=HomePage::FIRST;return true;}
    if(_page==HomePage::FIRST && (key==KEY_ENTER || key==KEY_SELECT || key==KEY_UP || key==KEY_DOWN)){_classic_menu=true;return true;}
#else
    if(key==KEY_CANCEL) {_page=HomePage::FIRST;return true;}
#endif
    if(key==KEY_ENTER && _page==HomePage::MESSAGES) {messages.open();return true;}
    if(key==KEY_ENTER && _page==HomePage::TIMEZONE) {_node_prefs->meshigotchi_timezone^=1;the_mesh.savePrefs();return true;}
#if !defined(BRAND_CLASSIC_UI) || defined(MESHIGOTCHI_GOTCHI_BOARD)
    if(key==KEY_ENTER && _page==HomePage::ANIMATIONS) {_node_prefs->meshigotchi_animations^=1;the_mesh.savePrefs();return true;}
#endif

    if (c == KEY_LEFT || c == KEY_PREV) {
      _page = (_page + HomePage::Count - 1) % HomePage::Count;
      return true;
    }
    if (c == KEY_NEXT || c == KEY_RIGHT) {
      _page = (_page + 1) % HomePage::Count;
      if (_page == HomePage::RECENT) {
        _task->showAlert("Recent adverts", 800);
      }
#if UI_DISCOVER_SCREEN
      if (_page == HomePage::DISCOVERY) {
        _task->showAlert("Repeater disc", 800);
      }
#endif
      return true;
    }
    if (c == KEY_ENTER && _page == HomePage::BLUETOOTH) {
      if (_task->isBluetoothEnabled()) {  // toggle Bluetooth on/off
        _task->disableBluetooth();
      } else {
        _task->enableBluetooth();
      }
      return true;
    }
    if (c == KEY_ENTER && _page == HomePage::ADVERT) {
      _task->notify(UIEventType::ack);
      if (the_mesh.advert()) {
        _task->showAlert("Advert sent!", 1000);
      } else {
        _task->showAlert("Advert failed..", 1000);
      }
      return true;
    }
#if ENV_INCLUDE_GPS == 1
    if (c == KEY_ENTER && _page == HomePage::GPS) {
      _task->toggleGPS();
      return true;
    }
#endif
#if UI_SENSORS_PAGE == 1
    if (c == KEY_ENTER && _page == HomePage::SENSORS) {
      _task->toggleGPS();
      next_sensors_refresh=0;
      return true;
    }
#endif
#if UI_DISCOVER_SCREEN
    if (c == KEY_ENTER && _page == HomePage::DISCOVERY) {
      if (millis() > discovery_req_time + 5000) { // rate limiter
        sendDiscoverRequest();
      }
      return true;
    }
    if (c == KEY_SELECT && _page == HomePage::DISCOVERY) {
      discovery_disp_names = !discovery_disp_names;
      return true;
    }
#endif
#ifndef UI_NO_HIBERNATE
    if (c == KEY_ENTER && _page == HomePage::SHUTDOWN) {
      _shutdown_init = true;  // need to wait for button to be released
      return true;
    }
#endif
    return false;
  }
};

#ifndef UI_MSG_PREVIEW_SIZE
  #define UI_MSG_PREVIEW_SIZE 78
#endif

class MsgPreviewScreen : public UIScreen {
  UITask* _task;
  mesh::RTCClock* _rtc;

  struct MsgEntry {
    uint32_t timestamp;
    char origin[62];
    char msg[UI_MSG_PREVIEW_SIZE];
  };
  #define MAX_UNREAD_MSGS   32
  int num_unread;
  int head = MAX_UNREAD_MSGS - 1; // index of latest unread message
  MsgEntry unread[MAX_UNREAD_MSGS];

public:
  MsgPreviewScreen(UITask* task, mesh::RTCClock* rtc) : _task(task), _rtc(rtc) { num_unread = 0; }

  void addPreview(uint8_t path_len, const char* from_name, const char* msg) {
    head = (head + 1) % MAX_UNREAD_MSGS;
    if (num_unread < MAX_UNREAD_MSGS) num_unread++;

    auto p = &unread[head];
    p->timestamp = _rtc->getCurrentTime();
    if (path_len == 0xFF) {
      snprintf(p->origin,sizeof(p->origin), "(D) %.48s:", from_name);
    } else {
      snprintf(p->origin,sizeof(p->origin), "(%d) %.48s:", (uint32_t) path_len, from_name);
    }
    StrHelper::strncpy(p->msg, msg, sizeof(p->msg));
  }

  int render(DisplayDriver& display) override {
    char tmp[16];
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setColor(UIColor::corp_blue);
    sprintf(tmp, "Unread: %d", num_unread);
    display.print(tmp);

    auto p = &unread[head];

    int secs = _rtc->getCurrentTime() - p->timestamp;
    if (secs < 60) {
      sprintf(tmp, "%ds", secs);
    } else if (secs < 60*60) {
      sprintf(tmp, "%dm", secs / 60);
    } else {
      sprintf(tmp, "%dh", secs / (60*60));
    }
    display.setCursor(display.width() - display.getTextWidth(tmp) - 2, 0);
    display.print(tmp);

    display.drawRect(0, 11, display.width(), 1);  // horiz line

    display.setCursor(0, 14);
    display.setColor(UIColor::secondary_txt);
    char filtered_origin[sizeof(p->origin)];
    display.translateUTF8ToBlocks(filtered_origin, p->origin, sizeof(filtered_origin));
    display.drawTextEllipsized(0,14,display.width(),filtered_origin);

    display.setCursor(0, 25);
    display.setColor(UIColor::primary_txt);
    char filtered_msg[sizeof(p->msg)];
    display.translateUTF8ToBlocks(filtered_msg, p->msg, sizeof(filtered_msg));
    display.printWordWrap(filtered_msg, display.width());

#if AUTO_OFF_MILLIS==0 // probably e-ink
    return 10000; // 10 s
#else
    return 1000;  // next render after 1000 ms
#endif
  }

  bool handleInput(char c) override {
    if(c==KEY_CANCEL) {_task->gotoHomeScreen();return true;}
    if (c == KEY_NEXT || c == KEY_RIGHT) {
      head = (head + MAX_UNREAD_MSGS - 1) % MAX_UNREAD_MSGS;
      num_unread--;
      if (num_unread == 0) {
        _task->gotoHomeScreen();
      }
      return true;
    }
    if (c == KEY_ENTER) {
      num_unread = 0;  // clear unread queue
      _task->gotoHomeScreen();
      return true;
    }
    return false;
  }
};

