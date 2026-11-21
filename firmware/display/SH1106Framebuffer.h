#pragma once
#include <Adafruit_SH110X.h>

namespace meshigotchi {
// The shared renderer writes the existing buffer directly, bypassing
// Adafruit_GrayOLED::drawPixel() and its dirty-window bookkeeping.
// Each rendered frame replaces the full buffer, so invalidate the complete
// 128x64 area before asking the original driver to transfer it.
class SH1106Framebuffer : public Adafruit_SH1106G {
public:
  using Adafruit_SH1106G::Adafruit_SH1106G;
  void flushFramebuffer() {
    window_x1 = 0;
    window_y1 = 0;
    window_x2 = WIDTH - 1;
    window_y2 = HEIGHT - 1;
    display();
  }
};
}
