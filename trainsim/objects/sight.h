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
    printf("    arrived at sighting point to %p\n",sig);
    if (t.get_dir() == this->sig->dir)
      t.can_see(this->sig, this->dist);
  }
};


#endif