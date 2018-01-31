#ifndef __INFRASTRUCTURE_OBJECT_H
#define __INFRASTRUCTURE_OBJECT_H

#include "../inputs.h"
#include "train.h"

class ISObj;
struct Link {
  ISObj *obj;
  double length;
};

inline bool operator==(const Link& lhs, const Link& rhs)
{
    return lhs.obj == rhs.obj && lhs.length == rhs.length;
}

static const Link NoLink = {nullptr, 0.0};
static const Link BoundaryLink = {nullptr, std::numeric_limits<double>::infinity()};

class ISObj {
public:
  string name;
  Link down = NoLink;
  Link up = NoLink;

  virtual void arrive_front(Train &t){};
  virtual void arrive_back(Train &t){};
  virtual Link next(Direction dir) {
    if (dir == Direction::Up)
      return this->up;
    if (dir == Direction::Down)
      return this->down;
    return {nullptr, 0.0}; // Unreachable
  }
};


#endif