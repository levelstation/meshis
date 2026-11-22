#pragma once
#include <stdint.h>
#include <array>
#include <cstring>
#define SSD1306_SWITCHCAPVCC 2
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_DISPLAYOFF 0xAE
struct TestWire {int error=0;void beginTransmission(uint8_t address){last=address;}int endTransmission(){return error;}int last=0;};
static TestWire Wire;
static bool testBegin=true;static unsigned transfers=0;static std::array<uint8_t,1024> transferred;
class Adafruit_SSD1306 {
 std::array<uint8_t,1024> bytes{};
public:
 Adafruit_SSD1306(int width,int height,TestWire*,int){if(width!=128||height!=64)__builtin_trap();}
 bool begin(int,uint8_t,bool,bool){return testBegin;}
 uint8_t* getBuffer(){return bytes.data();}
 void ssd1306_command(uint8_t){}
 void display(){transferred=bytes;++transfers;}
};
