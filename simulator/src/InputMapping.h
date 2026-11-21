#pragma once
#include <cstdint>
inline uint8_t firmwareKeyForMacCode(unsigned code) {
  switch(code){case 123:return 0xb4;case 124:return 0xb7;case 125:return 0xb6;case 126:return 0xb5;
    case 36:case 76:return 13;case 53:return 27;default:return 0;}
}
inline int simulatorZoom(int selection){const int values[]={4,8,12};return selection>=0 && selection<3?values[selection]:8;}
inline int simulatorHz(int selection){const int values[]={1,5,15,30,60};return selection>=0 && selection<5?values[selection]:15;}

inline uint8_t firmwareKeyForMacEvent(unsigned code,bool repeat){const uint8_t key=firmwareKeyForMacCode(code);return repeat && key==13?0:key;}
