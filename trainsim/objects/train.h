#ifndef __TRAIN_H
#define __TRAIN_H

#include "../simcpp.h"
class Signal;

class Train : public Process {
    public:
        virtual void add_sight(Signal *s, double d) = 0;
        virtual Direction get_dir() = 0;
};

#endif