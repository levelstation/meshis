#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#define PROGMEM
class __FlashStringHelper;
class String:public std::string {public:using std::string::string;unsigned length()const{return size();}};
uint32_t millis();
inline double radians(double degrees){return degrees*3.14159265358979323846/180.;}
