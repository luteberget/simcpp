#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "infrastructure_object.h"
#include "detector.h"
#include "../history.h"

class Signal : protected EnvObj, public ISObj {
public:
  Direction dir;
  Detector *detector;
  OutputWriter* out;
  Signal(Sim s, OutputWriter* o, Direction dir) : EnvObj(s), dir(dir), out(o) {}
  OBSERVABLE_PROPERTY(bool, green, false)
  OBSERVABLE_PROPERTY(double, authority, 0.0)
};

#endif