#ifndef __SIMOBJ_H
#define __SIMOBJ_H

#include "simcpp.h"

#define OBSERVABLE_PROPERTY(TYP, NAM, VAL)                      \
private:                                                        \
  TYP NAM = VAL;                                                \
                                                                \
public:                                                         \
  shared_ptr<Event> NAM##_event = std::make_shared<Event>(env); \
  TYP get_##NAM() { return this->NAM; }                         \
  void set_##NAM(TYP v)                                         \
  {                                                             \
    this->NAM = v;                                              \
    this->env->schedule(this->NAM##_event);                     \
    this->NAM##_event = std::make_shared<Event>(env);           \
  }

class EnvObj
{
protected:
  shared_ptr<Simulation> env;

public:
  EnvObj(shared_ptr<Simulation> s) : env(s) {}
};

#endif