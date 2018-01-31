#ifndef __BOUNDARY_H
#define __BOUNDARY_H

#include "infrastructure_object.h"
class Boundary : public ISObj {
    public:
    Boundary() {
        up = BoundaryLink; down = BoundaryLink;
    }

    virtual void arrive_back(Train& t) override {
        if(t.get_dir() == Direction::Up && up == BoundaryLink) {
            t.cleared_boundary();
        }
        if(t.get_dir() == Direction::Down && down == BoundaryLink) {
            t.cleared_boundary();
        }
    }
};

#endif