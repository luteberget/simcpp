#ifndef __INFRASTRUCTURE_OBJECT_H
#define __INFRASTRUCTURE_OBJECT_H

#include "../inputs.h"
#include "train.h"

class ISObj;
struct Link {
  ISObj *obj = nullptr;
  double length;
};

class ISObj {
public:
  Link down;
  Link up;

  virtual void arrive_front(Train &t){};
  virtual void arrive_back(Train &t){};
  virtual Link *next(Direction dir) {
    if (dir == Direction::Up)
      return &this->up;
    if (dir == Direction::Down)
      return &this->down;
    return nullptr;
  }
};


#endif