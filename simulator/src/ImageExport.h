#pragma once
#include <string>
#include <cstdint>
bool writePng(const uint8_t* pageBuffer,const std::string& path,int scale=1);
int runTests(const std::string& output,bool bless);

bool verifyPng(const uint8_t* pageBuffer,const std::string& path,int scale);

bool writeLionTriptych(const uint8_t* a,const uint8_t* b,const uint8_t* c,const std::string& path);
