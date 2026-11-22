#pragma once
struct RefCountedDigitalPin {int claims=0;void claim(){++claims;}void release(){--claims;}};
