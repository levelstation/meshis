#pragma once
namespace meshis {
inline const char* inputHint(const char* standard,const char* prg){
#if defined(MESHIS_HELTEC_PRG)
 (void)standard;return prg;
#else
 (void)prg;return standard;
#endif
}
}
