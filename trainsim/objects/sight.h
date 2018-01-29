#ifndef __SIGHT_H
#define __SIGHT_H

#include "infrastructure_object.h"
#include "signal.h"

class Sight : public ISObj {
  double dist;

public:
  Signal *sig;
  Sight(double dist) : dist(dist) {}
  void arrive_front(Train &t) override {
    if (t.get_dir() == this->sig->dir)
      t.add_sight(this->sig, this->dist);
  }
};


#endif