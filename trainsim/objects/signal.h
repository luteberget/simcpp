#ifndef __SIGNAL_H
#define __SIGNAL_H

#include "infrastructure_object.h"
#include "detector.h"


class Signal : protected EnvObj, public ISObj {
public:
  Direction dir;
  Detector *detector;
  Signal(Sim s, Direction dir) : EnvObj(s), dir(dir) {}
  OBSERVABLE_PROPERTY(bool, green, false)
  OBSERVABLE_PROPERTY(double, authority, 0.0)
};

#endif