#pragma once
#include <algorithm>
#include <cmath>
struct ViewportLayout {int scale;int x,y,width,height;};
// The framebuffer remains 128x64. Requested scale is a ceiling, never a crop.
inline ViewportLayout fitOLED(int availableWidth,int availableHeight,int requestedScale=0,int padding=20) {
  const int fit=std::max(1,std::min(std::max(0,availableWidth-2*padding)/128,std::max(0,availableHeight-2*padding)/64));
  const int scale=requestedScale>0?std::min(requestedScale,fit):std::min(fit,12);
  return {scale,(availableWidth-128*scale)/2,(availableHeight-64*scale)/2,128*scale,64*scale};
}
